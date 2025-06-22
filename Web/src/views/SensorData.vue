<template>
  <div>
    <h2>📊 传感器数据</h2>
    <button @click="fetchData">🔄 手动刷新</button>

    <!-- 搜索筛选区域 -->
    <div class="search-panel">
      <!-- 快速搜索 -->
      <div class="search-row">
        <label>
          快速搜索：
          <input 
            type="text" 
            v-model="quickSearch" 
            placeholder="输入距离或光照数值"
          />
        </label>
        <span class="search-tip">实时搜索，无需点击按钮</span>
      </div>

      <!-- 详细筛选 -->
      <div class="filters-grid">
        <div class="filter-group">
          <label>开始时间：</label>
          <input type="datetime-local" v-model="filterStartTime" />
        </div>

        <div class="filter-group">
          <label>结束时间：</label>
          <input type="datetime-local" v-model="filterEndTime" />
        </div>

        <div class="filter-group">
          <label>距离范围：</label>
          <input type="number" v-model.number="filterTempMin" placeholder="最小" />
          <span> - </span>
          <input type="number" v-model.number="filterTempMax" placeholder="最大" />
        </div>

        <div class="filter-group">
          <label>光照范围：</label>
          <input type="number" v-model.number="filterLightMin" placeholder="最小" />
          <span> - </span>
          <input type="number" v-model.number="filterLightMax" placeholder="最大" />
        </div>
      </div>

      <!-- 快捷操作 -->
      <div class="quick-actions">
        <button @click="setTimeRange('today')">今天</button>
        <button @click="setTimeRange('week')">最近7天</button>
        <button @click="clearFilter">清除筛选</button>
        <span class="result-count">
          找到 {{ filteredData.length }} 条记录 
          {{ filteredData.length !== allData.length ? `/ 共${allData.length}条` : '' }}
        </span>
      </div>
    </div>

    <!-- 数据表格 -->
    <table v-if="pagedData.length" class="sensor-table">
      <thead>
        <tr>
          <th @click="sortBy('temperature')" class="sortable">
            距离 (cm) {{ getSortIcon('temperature') }}
          </th>
          <th @click="sortBy('light')" class="sortable">
            光照 (lux) {{ getSortIcon('light') }}
          </th>
          <th @click="sortBy('timestamp')" class="sortable">
            时间 {{ getSortIcon('timestamp') }}
          </th>
        </tr>
      </thead>
      <tbody>
        <tr v-for="(item, index) in pagedData" :key="index">
          <td>{{ item.temperature }}</td>
          <td>{{ item.light }}</td>
          <td>{{ formatTime(item.timestamp) }}</td>
        </tr>
      </tbody>
    </table>

    <p v-else class="no-data">
      {{ allData.length === 0 ? '暂无数据，请点击"手动刷新"' : '没有符合条件的数据，请调整筛选条件' }}
    </p>

    <!-- 分页控制 -->
    <div v-if="totalPages > 1" class="pagination">
      <button @click="currentPage = 1" :disabled="currentPage === 1">首页</button>
      <button @click="prevPage" :disabled="currentPage === 1">上一页</button>
      <span>第 {{ currentPage }} 页 / 共 {{ totalPages }} 页</span>
      <button @click="nextPage" :disabled="currentPage === totalPages">下一页</button>
      <button @click="currentPage = totalPages" :disabled="currentPage === totalPages">末页</button>
      
      <!-- 每页显示数量选择 -->
      <label style="margin-left: 20px;">
        每页显示：
        <select v-model="pageSize" @change="currentPage = 1">
          <option value="10">10条</option>
          <option value="20">20条</option>
          <option value="50">50条</option>
          <option value="100">100条</option>
        </select>
      </label>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, computed, watch } from 'vue';
import axios from 'axios';

const allData = ref([]);
const filteredData = ref([]);
const currentPage = ref(1);
const pageSize = ref(10);

// 筛选条件
const quickSearch = ref('');
const filterStartTime = ref('');
const filterEndTime = ref('');
const filterTempMin = ref(null);
const filterTempMax = ref(null);
const filterLightMin = ref(null);
const filterLightMax = ref(null);

// 排序
const sortField = ref('timestamp');
const sortOrder = ref('desc');

const fetchData = async () => {
  try {
    const res = await axios.get('http://192.168.97.65:8000/api/sensor-data');
    if (res.data && Array.isArray(res.data.data)) {
      allData.value = res.data.data.map(item => ({
        temperature: item.temperature,
        light: item.light,
        timestamp: item.timestamp,
      }));
      
      applyFiltersAndSort();
    }
  } catch (err) {
    console.error('获取数据失败', err);
  }
};

function formatTime(isoString) {
  const date = new Date(isoString);
  return date.toLocaleString('zh-CN');
}

// 应用筛选和排序
const applyFiltersAndSort = () => {
  let filtered = allData.value.filter(item => {
    // 快速搜索
    if (quickSearch.value) {
      const search = quickSearch.value.toString();
      const tempMatch = item.temperature.toString().includes(search);
      const lightMatch = item.light.toString().includes(search);
      if (!tempMatch && !lightMatch) return false;
    }

    // 时间范围筛选
    if (filterStartTime.value || filterEndTime.value) {
      const itemTime = new Date(item.timestamp);
      if (filterStartTime.value && itemTime < new Date(filterStartTime.value)) return false;
      if (filterEndTime.value && itemTime > new Date(filterEndTime.value)) return false;
    }

    // 距离范围筛选
    if (filterTempMin.value !== null && item.temperature < filterTempMin.value) return false;
    if (filterTempMax.value !== null && item.temperature > filterTempMax.value) return false;

    // 光照范围筛选
    if (filterLightMin.value !== null && item.light < filterLightMin.value) return false;
    if (filterLightMax.value !== null && item.light > filterLightMax.value) return false;

    return true;
  });

  // 排序
  filtered.sort((a, b) => {
    let aVal = a[sortField.value];
    let bVal = b[sortField.value];
    
    if (sortField.value === 'timestamp') {
      aVal = new Date(aVal);
      bVal = new Date(bVal);
    }
    
    const result = aVal > bVal ? 1 : -1;
    return sortOrder.value === 'asc' ? result : -result;
  });

  filteredData.value = filtered;
  if (currentPage.value > totalPages.value && totalPages.value > 0) {
    currentPage.value = 1;
  }
};

// 监听所有筛选条件变化，实时筛选
watch(
  [quickSearch, filterStartTime, filterEndTime, filterTempMin, filterTempMax, filterLightMin, filterLightMax, sortField, sortOrder],
  applyFiltersAndSort
);

const totalPages = computed(() => Math.ceil(filteredData.value.length / pageSize.value));

const pagedData = computed(() => {
  const start = (currentPage.value - 1) * pageSize.value;
  return filteredData.value.slice(start, start + pageSize.value);
});

function prevPage() {
  if (currentPage.value > 1) currentPage.value--;
}

function nextPage() {
  if (currentPage.value < totalPages.value) currentPage.value++;
}

// 排序功能
function sortBy(field) {
  if (sortField.value === field) {
    sortOrder.value = sortOrder.value === 'asc' ? 'desc' : 'asc';
  } else {
    sortField.value = field;
    sortOrder.value = field === 'timestamp' ? 'desc' : 'asc';
  }
}

function getSortIcon(field) {
  if (sortField.value !== field) return '';
  return sortOrder.value === 'asc' ? '↑' : '↓';
}

// 时间范围快捷设置
function setTimeRange(type) {
  const now = new Date();
  const today = new Date(now.getFullYear(), now.getMonth(), now.getDate());
  
  if (type === 'today') {
    filterStartTime.value = today.toISOString().slice(0, 16);
    filterEndTime.value = now.toISOString().slice(0, 16);
  } else if (type === 'week') {
    const weekAgo = new Date(today);
    weekAgo.setDate(today.getDate() - 7);
    filterStartTime.value = weekAgo.toISOString().slice(0, 16);
    filterEndTime.value = now.toISOString().slice(0, 16);
  }
}

// 清除筛选
function clearFilter() {
  quickSearch.value = '';
  filterStartTime.value = '';
  filterEndTime.value = '';
  filterTempMin.value = null;
  filterTempMax.value = null;
  filterLightMin.value = null;
  filterLightMax.value = null;
}

onMounted(() => {
  fetchData();
});
</script>

<style scoped>
/* 基础样式 */
button {
  padding: 8px 16px;
  margin: 4px;
  cursor: pointer;
  border: 1px solid #ccc;
  background: #fff;
}

button:hover {
  background: #f5f5f5;
}

button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

input, select {
  padding: 6px 8px;
  border: 1px solid #ccc;
  margin: 2px;
}

/* 搜索面板 */
.search-panel {
  background: #f8f9fa;
  border: 1px solid #dee2e6;
  padding: 15px;
  margin: 15px 0;
}

.search-row {
  margin-bottom: 15px;
}

.search-row label {
  font-weight: bold;
  margin-right: 10px;
}

.search-tip {
  color: #666;
  font-size: 12px;
  margin-left: 10px;
}

.filters-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  gap: 15px;
  margin-bottom: 15px;
}

.filter-group {
  display: flex;
  align-items: center;
  gap: 5px;
}

.filter-group label {
  min-width: 80px;
  font-weight: bold;
}

.filter-group input {
  flex: 1;
  max-width: 100px;
}

.quick-actions {
  display: flex;
  align-items: center;
  gap: 10px;
  padding-top: 10px;
  border-top: 1px solid #dee2e6;
}

.result-count {
  margin-left: auto;
  font-weight: bold;
  color: #007bff;
}

/* 表格样式 */
.sensor-table {
  width: 100%;
  border-collapse: collapse;
  margin: 15px 0;
}

.sensor-table th,
.sensor-table td {
  border: 1px solid #ccc;
  padding: 10px;
  text-align: center;
}

.sensor-table th {
  background-color: #f2f2f2;
  font-weight: bold;
}

.sortable {
  cursor: pointer;
  user-select: none;
}

.sortable:hover {
  background-color: #e9ecef;
}

.sensor-table tr:nth-child(even) {
  background-color: #f9f9f9;
}

/* 无数据提示 */
.no-data {
  text-align: center;
  padding: 20px;
  color: #666;
  background: #f8f9fa;
  border: 1px solid #dee2e6;
}

/* 分页样式 */
.pagination {
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 10px;
  margin: 20px 0;
  padding: 15px;
  background: #f8f9fa;
  border: 1px solid #dee2e6;
}

/* 响应式 */
@media (max-width: 768px) {
  .filters-grid {
    grid-template-columns: 1fr;
  }
  
  .quick-actions {
    flex-wrap: wrap;
  }
  
  .result-count {
    margin-left: 0;
    margin-top: 10px;
  }
  
  .pagination {
    flex-wrap: wrap;
    gap: 5px;
  }
}
</style>