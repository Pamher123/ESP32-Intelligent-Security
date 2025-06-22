import express from 'express';
import cors from 'cors';   
import db from './mysql.js';
import multer from 'multer'
import { fileURLToPath } from 'url'
import path from 'path'
import fs from 'fs'
const __filename = fileURLToPath(import.meta.url)
const __dirname = path.dirname(__filename)

let latestImage = null;         // 用于保存最新图像
let clients = [];               // 用于保存连接的客户端列表




const app = express();
const port = 8000;

app.use(cors({
  origin: 'http://localhost:8080'
}));
app.use(express.static('public'));
app.use(express.json());

app.use(express.urlencoded({
    extended: false
}));


//======================为ESP32提供的API========================
// 接收 ESP32 上传的图片
app.post('/upload', (req, res) => {
    let data = [];

    req.on('data', chunk => data.push(chunk));
    req.on('end', () => {
        const image = Buffer.concat(data);
        const filename = `image_${Date.now()}.bmp`;

        const sql = "INSERT INTO camera_images (image, filename) VALUES (?, ?)";
        db.querySql(sql, [image, filename], (err, results) => {
            if (err) {
                console.error('数据库插入失败:', err);
                return res.status(500).send('数据库错误');
            }
            res.sendStatus(200);
        });
    });
});

// 提供图片列表给网页
app.get('/images', (req, res) => {
  const page = parseInt(req.query.page) || 1;
  const pageSize = 10;
  const offset = (page - 1) * pageSize;
  
  // 先查询总数
  const countSql = 'SELECT COUNT(*) as total FROM camera_images';
  
  db.querySql(countSql, [], (err, countResults) => {
    if (err) {
      console.error('查询总数失败:', err);
      return res.status(500).send('查询总数失败');
    }
    
    const total = countResults[0].total;
    
    // 再查询分页数据
    const sql = `
      SELECT id, timestamp, TO_BASE64(image) AS image 
      FROM camera_images 
      ORDER BY id DESC 
      LIMIT ? OFFSET ?
    `;
    
    db.querySql(sql, [pageSize, offset], (err, results) => {
      if (err) {
        console.error('查询失败:', err);
        return res.status(500).send('查询失败');
      }
      
      res.json({
        data: results,
        total: total,
        page: page,
        pageSize: pageSize,
        hasMore: offset + results.length < total
      });
    });
  });
});

//查找图片
app.get('/images/search', (req, res) => {
  const page = parseInt(req.query.page) || 1;
  const pageSize = 10;
  const offset = (page - 1) * pageSize;
  
  const { start, end } = req.query;
  
  // 构建查询条件
  let whereClause = 'WHERE 1=1';
  const params = [];
  
  if (start) {
    whereClause += ' AND timestamp >= ?';
    params.push(start);
  }
  
  if (end) {
    whereClause += ' AND timestamp <= ?';
    params.push(end);
  }
  
  // 先查询总数
  const countSql = `SELECT COUNT(*) as total FROM camera_images ${whereClause}`;
  
  db.querySql(countSql, params, (err, countResults) => {
    if (err) {
      console.error('查询总数失败:', err);
      return res.status(500).send('查询总数失败');
    }
    
    const total = countResults[0].total;
    
    // 再查询分页数据
    const sql = `
      SELECT id, timestamp, TO_BASE64(image) AS image, filename 
      FROM camera_images 
      ${whereClause}
      ORDER BY id DESC 
      LIMIT ? OFFSET ?
    `;
    
    const dataParams = [...params, pageSize, offset];
    
    db.querySql(sql, dataParams, (err, results) => {
      if (err) {
        console.error('查询失败:', err);
        return res.status(500).send('查询失败');
      }
      
      res.json({
        data: results,
        total: total,
        page: page,
        pageSize: pageSize,
        hasMore: offset + results.length < total
      });
    });
  });
});

// =======================获取 sensor_readings 数据=============================
// 修改后的Node.js API - 返回对象格式而不是数组
app.get('/api/sensor-data', (req, res) => {
  const sql = 'SELECT light, distance, timestamp FROM sensor_readings ORDER BY id ASC';

  db.querySql(sql, (err, results) => {
    if (err) {
      console.error('查询出错:', err);
      return res.status(500).json({ error: '数据库查询失败' });
    }

    console.log('查询结果：', results);

    if (!results || results.length === 0) {
      return res.json({
        data: [],
        message: '暂无传感器数据'
      });
    }

    // 直接返回所有数据
    const responseData = results.map(row => ({
      temperature: row.distance || 0,
      light: row.light || 0,
      timestamp: row.timestamp || new Date().toISOString()
    }));

    res.json({ data: responseData });
  }); 
});


// =======================获取 sensor_readings 数据=============================

// 接收ESP32的流数据
app.post('/stream-upload', (req, res) => {
  let imageData = Buffer.alloc(0);
  
  req.on('data', (chunk) => {
    imageData = Buffer.concat([imageData, chunk]);
  });
  
  req.on('end', () => {
    latestImage = imageData;
    
    // 向所有连接的客户端发送新图片
    clients.forEach(client => {
      if (!client.destroyed) {
        client.write(`--frame\r\n`);
        client.write(`Content-Type: image/bmp\r\n`);
        client.write(`Content-Length: ${imageData.length}\r\n\r\n`);
        client.write(imageData);
        client.write(`\r\n`);
      }
    });
    
    res.sendStatus(200);
  });
});

//提供实时流
app.get('/stream', (req, res) => {
  res.writeHead(200, {
    'Content-Type': 'multipart/x-mixed-replace; boundary=frame',
    'Connection': 'keep-alive',
    'Cache-Control': 'no-cache'
  });
  
  clients.push(res);
  
  // 如果有最新图片，立即发送
  if (latestImage) {
    res.write(`--frame\r\n`);
    res.write(`Content-Type: image/bmp\r\n`);
    res.write(`Content-Length: ${latestImage.length}\r\n\r\n`);
    res.write(latestImage);
    res.write(`\r\n`);
  }
  
  // 清理断开的连接
  res.on('close', () => {
    const index = clients.indexOf(res);
    if (index > -1) {
      clients.splice(index, 1);
    }
  });
});
//======================为ESP32提供的API========================

app.listen(8000, '0.0.0.0', () => {
  console.log("Server is running at http://localhost:8000...");
});