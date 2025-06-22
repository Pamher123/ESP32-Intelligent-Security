<template>
  <view class="container">
    <text class="title">📊 传感器数据</text>
    <button @click="fetchData" class="btn">🔄 手动刷新</button>

    <!-- 搜索筛选区域 -->
    <view class="search-panel">
      <!-- 快速搜索 -->
      <view class="search-row">
        <text class="label">快速搜索：</text>
        <input 
          class="input" 
          v-model="quickSearch" 
          placeholder="输入距离或光照数值"
          confirm-type="done"
          @confirm="applyFiltersAndSort"
        />
        <text class="search-tip">实时搜索，无需点击按钮</text>
      </view>

      <!-- 详细筛选 -->
      <view class="filters-grid">
        <view class="filter-group">
          <text class="label">开始时间：</text>
          <input 
            type="text" 
            v-model="filterStartTime" 
            placeholder="YYYY-MM-DD HH:mm" 
            @focus="showStartPicker = true"
            readonly
          />
          <uni-datetime-picker 
            v-if="showStartPicker"
            type="datetime"
            @change="val => { filterStartTime = val.detail.value; showStartPicker = false; applyFiltersAndSort(); }"
            @cancel="showStartPicker = false"
          />
        </view>

        <view class="filter-group">
          <text class="label">结束时间：</text>
          <input 
            type="text" 
            v-model="filterEndTime" 
            placeholder="YYYY-MM-DD HH:mm" 
            @focus="showEndPicker = true"
            readonly
          />
          <uni-datetime-picker 
            v-if="showEndPicker"
            type="datetime"
            @change="val => { filterEndTime = val.detail.value; showEndPicker = false; applyFiltersAndSort(); }"
            @cancel="showEndPicker = false"
          />
        </view>

        <view class="filter-group">
          <text class="label">距离范围：</text>
          <input class="input-number" type="number" v-model.number="filterTempMin" placeholder="最小" />
          <text> - </text>
          <input class="input-number" type="number" v-model.number="filterTempMax" placeholder="最大" />
        </view>

        <view class="filter-group">
          <text class="label">光照范围：</text>
          <input class="input-number" type="number" v-model.number="filterLightMin" placeholder="最小" />
          <text> - </text>
          <input class="input-number" type="number" v-model.number="filterLightMax" placeholder="最大" />
        </view>
      </view>

      <!-- 快捷操作 -->
      <view class="quick-actions">
        <button @click="setTimeRange('today')" class="btn">今天</button>
        <button @click="setTimeRange('week')" class="btn">最近7天</button>
        <button @click="clearFilter" class="btn">清除筛选</button>
        <text class="result-count">
          找到 {{ filteredData.length }} 条记录
          <text v-if="filteredData.length !== allData.length"> / 共{{ allData.length }}条</text>
        </text>
      </view>
    </view>

    <!-- 数据表格 -->
    <view v-if="pagedData.length" class="sensor-table">
      <view class="thead">
        <view class="th sortable" @click="sortBy('temperature')">
          距离 (cm) {{ getSortIcon('temperature') }}
        </view>
        <view class="th sortable" @click="sortBy('light')">
          光照 (lux) {{ getSortIcon('light') }}
        </view>
        <view class="th sortable" @click="sortBy('timestamp')">
          时间 {{ getSortIcon('timestamp') }}
        </view>
      </view>
      <scroll-view class="tbody" scroll-y style="max-height: 400rpx;">
        <view class="tr" v-for="(item, index) in pagedData" :key="index">
          <view class="td">{{ item.temperature }}</view>
          <view class="td">{{ item.light }}</view>
          <view class="td">{{ formatTime(item.timestamp) }}</view>
        </view>
      </scroll-view>
    </view>

    <text v-else class="no-data">
      {{ allData.length === 0 ? '暂无数据，请点击"手动刷新"' : '没有符合条件的数据，请调整筛选条件' }}
    </text>

    <!-- 分页控制 -->
    <view v-if="totalPages > 1" class="pagination">
      <button @click="currentPage = 1" :disabled="currentPage === 1" class="btn">首页</button>
      <button @click="prevPage" :disabled="currentPage === 1" class="btn">上一页</button>
      <text>第 {{ currentPage }} 页 / 共 {{ totalPages }} 页</text>
      <button @click="nextPage" :disabled="currentPage === totalPages" class="btn">下一页</button>
      <button @click="currentPage = totalPages" :disabled="currentPage === totalPages" class="btn">末页</button>
      
      <text style="margin-left: 20rpx;">
        每页显示：
        <picker mode="selector" :range="pageSizeOptions" :value="pageSizeIndex" @change="changePageSize">
          <view class="picker">{{ pageSize }}条</view>
        </picker>
      </text>
    </view>
  </view>
</template>

<script setup>
import { ref, onMounted, computed, watch } from 'vue';

const allData = ref([]);
const filteredData = ref([]);
const currentPage = ref(1);
const pageSizeIndex = ref(0);
const pageSizeOptions = [10, 20, 50, 100];
const pageSize = ref(pageSizeOptions[pageSizeIndex.value]);

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

// 时间选择器控制
const showStartPicker = ref(false);
const showEndPicker = ref(false);

// uni-app 请求替代 axios
const fetchData = () => {
  uni.request({
    url: 'http://192.168.97.65:8000/api/sensor-data',
    method: 'GET',
    success: (res) => {
      if (res.data && Array.isArray(res.data.data)) {
        allData.value = res.data.data.map(item => ({
          temperature: item.temperature,
          light: item.light,
          timestamp: item.timestamp,
        }));
        applyFiltersAndSort();
      }
    },
    fail: (err) => {
      console.error('获取数据失败', err);
    }
  });
};

function formatTime(isoString) {
  const date = new Date(isoString);
  return date.toLocaleString('zh-CN');
}

const applyFiltersAndSort = () => {
  let filtered = allData.value.filter(item => {
    if (quickSearch.value) {
      const search = quickSearch.value.toString();
      const tempMatch = item.temperature.toString().includes(search);
      const lightMatch = item.light.toString().includes(search);
      if (!tempMatch && !lightMatch) return false;
    }

    if (filterStartTime.value || filterEndTime.value) {
      const itemTime = new Date(item.timestamp);
      if (filterStartTime.value && itemTime < new Date(filterStartTime.value)) return false;
      if (filterEndTime.value && itemTime > new Date(filterEndTime.value)) return false;
    }

    if (filterTempMin.value !== null && item.temperature < filterTempMin.value) return false;
    if (filterTempMax.value !== null && item.temperature > filterTempMax.value) return false;

    if (filterLightMin.value !== null && item.light < filterLightMin.value) return false;
    if (filterLightMax.value !== null && item.light > filterLightMax.value) return false;

    return true;
  });

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
  return sortOrder.value === 'asc' ? '⬆️' : '⬇️';
}

function setTimeRange(range) {
  const now = new Date();
  let start, end;

  switch (range) {
    case 'today':
      start = new Date(now.getFullYear(), now.getMonth(), now.getDate());
      end = new Date(start.getTime() + 86400000 - 1);
      break;
    case 'week':
      start = new Date(now.getTime() - 7 * 86400000);
      end = now;
      break;
    default:
      start = null; end = null;
  }

  filterStartTime.value = start ? start.toISOString().slice(0, 16) : '';
  filterEndTime.value = end ? end.toISOString().slice(0, 16) : '';
  applyFiltersAndSort();
}

function clearFilter() {
  quickSearch.value = '';
  filterStartTime.value = '';
  filterEndTime.value = '';
  filterTempMin.value = null;
  filterTempMax.value = null;
  filterLightMin.value = null;
  filterLightMax.value = null;
  applyFiltersAndSort();
}

function changePageSize(e) {
  pageSizeIndex.value = e.detail.value;
  pageSize.value = pageSizeOptions[e.detail.value];
  currentPage.value = 1;
  applyFiltersAndSort();
}

onMounted(() => {
  fetchData();
});
</script>

<style scoped>
.container {
  padding: 20rpx;
  background-color: #f9f9f9;
  min-height: 100vh;
}

.title {
  font-size: 32rpx;
  font-weight: bold;
  margin-bottom: 20rpx;
}

.btn {
  background-color: #007aff;
  color: #fff;
  padding: 10rpx 20rpx;
  border-radius: 6rpx;
  margin: 5rpx 10rpx 10rpx 0;
  text-align: center;
}

.search-panel {
  background: #fff;
  padding: 20rpx;
  border-radius: 8rpx;
  margin-bottom: 20rpx;
}

.search-row {
  flex-direction: row;
  display: flex;
  align-items: center;
  margin-bottom: 10rpx;
}

.label {
  width: 80rpx;
  font-weight: 600;
}

.input {
  flex: 1;
  border: 1rpx solid #ccc;
  border-radius: 6rpx;
  padding: 8rpx;
  font-size: 28rpx;
  margin-right: 10rpx;
}

.search-tip {
  font-size: 24rpx;
  color: #666;
}

.filters-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 15rpx;
}

.filter-group {
  flex: 1 1 45%;
  display: flex;
  align-items: center;
  gap: 10rpx;
}

.input-number {
  width: 80rpx;
  padding: 6rpx;
  border: 1rpx solid #ccc;
  border-radius: 6rpx;
  font-size: 28rpx;
  text-align: center;
}

.quick-actions {
  margin-top: 10rpx;
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 15rpx;
}

.result-count {
  font-weight: 600;
  margin-left: auto;
}

.sensor-table {
  background: #fff;
  border-radius: 8rpx;
  overflow: hidden;
  border: 1rpx solid #ddd;
}

.thead {
  flex-direction: row;
  display: flex;
  background: #f0f0f0;
  font-weight: 700;
  font-size: 28rpx;
}

.th {
  flex: 1;
  padding: 10rpx;
  text-align: center;
  user-select: none;
}

.sortable {
  cursor: pointer;
}

.tbody {
  max-height: 400rpx;
}

.tr {
  flex-direction: row;
  display: flex;
  border-top: 1rpx solid #eee;
}

.td {
  flex: 1;
  padding: 10rpx;
  text-align: center;
  font-size: 28rpx;
  color: #333;
}

.no-data {
  text-align: center;
  font-size: 28rpx;
  color: #999;
  margin-top: 40rpx;
}

.pagination {
  margin-top: 15rpx;
  flex-direction: row;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 10rpx;
}

.picker {
  padding: 4rpx 12rpx;
  border: 1rpx solid #ccc;
  border-radius: 6rpx;
  font-size: 28rpx;
}
</style>