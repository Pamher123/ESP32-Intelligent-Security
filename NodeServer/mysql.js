import mysql from 'mysql';

const pool = mysql.createPool({
    host: 'localhost',
    port: 3306,
    user: 'root',
    password: '123123',
    database: 'yuemiao'
});

const querySql = (sql, params = null, callback) => {
    pool.getConnection((err, connection) => {
        if (err) {
            console.log(err);
            callback(err);
        } else {
            connection.query(sql, params, (err, results) => {
                if (err) {
                    console.error('执行 SQL 出错:', err);
                    callback(err);
                    connection.release();
                     return callback(err);
                }

                if (Array.isArray(results)) {
                    console.log(`查询到 ${results.length} 条数据`);
                    results.forEach(item => {
                        console.log(`id: ${item.id}, timestamp: ${item.timestamp}`);
                    });
                } else {
                    console.log('非查询结果:', results);
                }

                callback(null, results);
                connection.release();
            });
        }
    });
};


export default {
    querySql
};
