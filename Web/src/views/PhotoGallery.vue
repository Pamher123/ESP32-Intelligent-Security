<template>
  <div class="gallery-container">
    <h2>📷 查看相册</h2>

    <!-- 搜索筛选区域 -->
    <div class="search-panel">
      <div class="time-filters">
        <div class="filter-group">
          <label>开始时间：</label>
          <input type="datetime-local" v-model="startTime" />
        </div>

        <div class="filter-group">
          <label>结束时间：</label>
          <input type="datetime-local" v-model="endTime" />
        </div>
      </div>

      <div class="quick-actions">
        <button @click="setTimeRange('today')">今天</button>
        <button @click="setTimeRange('yesterday')">昨天</button>
        <button @click="setTimeRange('week')">最近7天</button>
        <button @click="setTimeRange('month')">最近30天</button>
        <button @click="clearFilter" class="clear-btn">清除筛选</button>
      </div>

      <div class="search-info">
        <span class="result-count">
          {{ loading ? '搜索中...' : `找到 ${totalImages} 张图片` }}
          {{ hasFilter ? '（已筛选）' : '' }}
        </span>
      </div>
    </div>

    <!-- 加载状态 -->
    <div v-if="loading" class="loading-state">
      <p>🔄 正在加载图片...</p>
    </div>

    <!-- 无图片状态 -->
    <div v-else-if="images.length === 0" class="no-images">
      <p>📷 {{ hasFilter ? '没有找到符合条件的图片' : '暂无图片' }}</p>
      <p v-if="hasFilter">请尝试调整时间范围或清除筛选条件</p>
    </div>

    <!-- 图片网格 -->
    <div v-else class="images-grid">
      <div 
        v-for="image in images" 
        :key="image.id" 
        class="image-card"
        @click="openImageModal(image)"
      >
        <img 
          :src="'data:image/jpeg;base64,' + image.image" 
          :alt="`图片 ${image.id}`"
          class="image-thumbnail"
          @error="handleImageError"
        />
        <div class="image-info">
          <p class="image-time">{{ formatTime(image.timestamp) }}</p>
        </div>
      </div>
    </div>

    <!-- 分页控制 -->
    <div v-if="!loading && images.length > 0" class="pagination-container">
      <div class="pagination">
        <button @click="goToPage(1)" :disabled="page === 1">首页</button>
        <button @click="prevPage" :disabled="page === 1">上一页</button>
        <span class="page-info">第 {{ page }} 页</span>
        <button @click="nextPage" :disabled="!hasNextPage">下一页</button>
        
        <div class="page-size-selector">
          <label>每页显示：</label>
          <select v-model="pageSize" @change="changePageSize">
            <option value="6">6张</option>
            <option value="10">10张</option>
            <option value="20">20张</option>
          </select>
        </div>
      </div>
      
      <div class="page-stats">
        显示第 {{ (page - 1) * pageSize + 1 }} - {{ Math.min(page * pageSize, totalImages) }} 张
      </div>
    </div>

    <!-- 图片弹窗 -->
    <div v-if="showModal" class="modal-overlay" @click="closeModal">
      <div class="modal-content" @click.stop>
        <button class="modal-close" @click="closeModal">✕</button>
        <img 
          :src="'data:image/jpeg;base64,' + selectedImage.image" 
          :alt="`图片 ${selectedImage.id}`"
          class="modal-image"
        />
        <div class="modal-info">
          <p><strong>拍摄时间：</strong>{{ formatTime(selectedImage.timestamp) }}</p>
          <p><strong>图片ID：</strong>{{ selectedImage.id }}</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import axios from 'axios';

export default {
  name: 'PhotoGallery',
  data() {
    return {
      images: [],
      loading: false,
      page: 1,
      pageSize: 10,
      totalImages: 0,
      startTime: '',
      endTime: '',
      showModal: false,
      selectedImage: null,
    };
  },
  computed: {
    hasFilter() {
      return this.startTime || this.endTime;
    },
    hasNextPage() {
      return this.images.length === this.pageSize;
    }
  },
  methods: {
    formatTime(timestamp) {
      const date = new Date(timestamp);
      return date.toLocaleString('zh-CN', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit'
      });
    },

    toBackendTimeFormat(localDateTimeStr) {
      if (!localDateTimeStr) return '';
      const dt = new Date(localDateTimeStr);
      if (isNaN(dt)) return '';

      const Y = dt.getFullYear();
      const M = dt.getMonth() + 1;
      const D = dt.getDate();
      const h = dt.getHours();
      const m = dt.getMinutes();
      const s = dt.getSeconds();

      return `${Y}/${M}/${D} ${this.pad(h)}:${this.pad(m)}:${this.pad(s)}`;
    },

    pad(n) {
      return n < 10 ? '0' + n : n;
    },

   async fetchImages() {
  this.loading = true;
  try {
    const params = { page: this.page };

    if (this.startTime) {
      params.start = this.toBackendTimeFormat(this.startTime);
    }
    if (this.endTime) {
      params.end = this.toBackendTimeFormat(this.endTime);
    }

    const response = await axios.get('http://192.168.97.65:8000/images/search', { params });
    
    // 修改为使用新的数据结构
    this.images = response.data.data || [];
    this.totalImages = response.data.total || 0;
    
  } catch (error) {
    console.error('获取相册图片失败:', error);
    this.images = [];
    this.totalImages = 0;
  } finally {
    this.loading = false;
  }
},

    // 分页操作
    goToPage(pageNum) {
      this.page = pageNum;
      this.fetchImages();
    },

    nextPage() {
      if (this.hasNextPage) {
        this.page++;
        this.fetchImages();
      }
    },

    prevPage() {
      if (this.page > 1) {
        this.page--;
        this.fetchImages();
      }
    },

    changePageSize() {
      this.page = 1;
      this.fetchImages();
    },

    // 时间范围快捷设置
   setTimeRange(type) {
  const now = new Date();
  const today = new Date(now.getFullYear(), now.getMonth(), now.getDate());
  
  this.endTime = this.toDateTimeLocal(now);
  
  switch (type) {
    case 'today': {
      this.startTime = this.toDateTimeLocal(today);
      break;
    }
    case 'yesterday': {
      const yesterday = new Date(today);
      yesterday.setDate(today.getDate() - 1);
      this.startTime = this.toDateTimeLocal(yesterday);
      this.endTime = this.toDateTimeLocal(today);
      break;
    }
    case 'week': {
      const weekAgo = new Date(today);
      weekAgo.setDate(today.getDate() - 7);
      this.startTime = this.toDateTimeLocal(weekAgo);
      break;
    }
    case 'month': {
      const monthAgo = new Date(today);
      monthAgo.setDate(today.getDate() - 30);
      this.startTime = this.toDateTimeLocal(monthAgo);
      break;
    }
  }
  
  this.page = 1;
  this.fetchImages();
},

    clearFilter() {
      this.startTime = '';
      this.endTime = '';
      this.page = 1;
      this.fetchImages();
    },

    toDateTimeLocal(dt) {
      const Y = dt.getFullYear();
      const M = (dt.getMonth() + 1).toString().padStart(2, '0');
      const D = dt.getDate().toString().padStart(2, '0');
      const h = dt.getHours().toString().padStart(2, '0');
      const m = dt.getMinutes().toString().padStart(2, '0');
      return `${Y}-${M}-${D}T${h}:${m}`;
    },

    // 图片弹窗
    openImageModal(image) {
      this.selectedImage = image;
      this.showModal = true;
      document.body.style.overflow = 'hidden';
    },

    closeModal() {
      this.showModal = false;
      this.selectedImage = null;
      document.body.style.overflow = '';
    },

    handleImageError(event) {
      event.target.src = 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMjAwIiBoZWlnaHQ9IjE2MCIgdmlld0JveD0iMCAwIDIwMCAxNjAiIGZpbGw9Im5vbmUiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+CjxyZWN0IHdpZHRoPSIyMDAiIGhlaWdodD0iMTYwIiBmaWxsPSIjRjNGNEY2Ii8+CjxwYXRoIGQ9Ik01MCA1MEwxNTAgMTEwTTUwIDExMEwxNTAgNTAiIHN0cm9rZT0iI0Q0RERFNiIgc3Ryb2tlLXdpZHRoPSIyIi8+Cjx0ZXh0IHg9IjEwMCIgeT0iMTMwIiBmaWxsPSIjOTBBNEFFIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIiBmb250LXNpemU9IjEyIj7lm77niYfliqDovb3lpLHotKU8L3RleHQ+Cjwvc3ZnPgo=';
    }
  },

  // 监听时间筛选条件变化，自动搜索
  watch: {
    startTime() {
      if (this.startTime) {
        this.page = 1;
        this.fetchImages();
      }
    },
    endTime() {
      if (this.endTime) {
        this.page = 1;
        this.fetchImages();
      }
    }
  },

  mounted() {
    this.fetchImages();
  },

  beforeUnmount() {
    document.body.style.overflow = '';
  }
};
</script>

<style scoped>
/* 基础样式 */
.gallery-container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 20px;
}

h2 {
  font-size: 24px;
  font-weight: bold;
  margin-bottom: 20px;
  color: #333;
}

/* 搜索面板 */
.search-panel {
  background: #f8f9fa;
  border: 1px solid #dee2e6;
  border-radius: 6px;
  padding: 20px;
  margin-bottom: 20px;
}

.time-filters {
  display: flex;
  gap: 20px;
  margin-bottom: 15px;
  flex-wrap: wrap;
}

.filter-group {
  display: flex;
  align-items: center;
  gap: 8px;
}

.filter-group label {
  font-weight: bold;
  color: #555;
  min-width: 70px;
}

.filter-group input {
  padding: 8px;
  border: 1px solid #ccc;
  border-radius: 4px;
  font-size: 14px;
}

.quick-actions {
  display: flex;
  gap: 10px;
  margin-bottom: 15px;
  flex-wrap: wrap;
}

.quick-actions button {
  padding: 8px 16px;
  border: 1px solid #007bff;
  background: #fff;
  color: #007bff;
  border-radius: 4px;
  cursor: pointer;
  font-size: 14px;
}

.quick-actions button:hover {
  background: #007bff;
  color: white;
}

.clear-btn {
  background: #dc3545 !important;
  color: white !important;
  border-color: #dc3545 !important;
}

.clear-btn:hover {
  background: #c82333 !important;
}

.search-info {
  padding-top: 10px;
  border-top: 1px solid #dee2e6;
}

.result-count {
  font-weight: bold;
  color: #007bff;
}

/* 状态显示 */
.loading-state, .no-images {
  text-align: center;
  padding: 40px;
  color: #666;
  background: #f8f9fa;
  border-radius: 6px;
  margin: 20px 0;
}

.no-images p:first-child {
  font-size: 18px;
  margin-bottom: 10px;
}

/* 图片网格 */
.images-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
  gap: 20px;
  margin: 20px 0;
}

.image-card {
  border: 1px solid #dee2e6;
  border-radius: 8px;
  overflow: hidden;
  cursor: pointer;
  background: white;
  box-shadow: 0 2px 4px rgba(0,0,0,0.1);
}

.image-card:hover {
  box-shadow: 0 4px 8px rgba(0,0,0,0.15);
}

.image-thumbnail {
  width: 100%;
  height: 200px;
  object-fit: cover;
  display: block;
}

.image-info {
  padding: 12px;
}

.image-time {
  margin: 0;
  font-size: 14px;
  color: #666;
  text-align: center;
}

/* 分页样式 */
.pagination-container {
  margin-top: 30px;
  padding-top: 20px;
  border-top: 1px solid #dee2e6;
}

.pagination {
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 15px;
  margin-bottom: 10px;
  flex-wrap: wrap;
}

.pagination button {
  padding: 8px 16px;
  border: 1px solid #ccc;
  background: white;
  cursor: pointer;
  border-radius: 4px;
}

.pagination button:hover:not(:disabled) {
  background: #f5f5f5;
}

.pagination button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.page-info {
  font-weight: bold;
  color: #333;
  margin: 0 10px;
}

.page-size-selector {
  display: flex;
  align-items: center;
  gap: 8px;
}

.page-size-selector select {
  padding: 6px;
  border: 1px solid #ccc;
  border-radius: 4px;
}

.page-stats {
  text-align: center;
  color: #666;
  font-size: 14px;
}

/* 图片弹窗 */
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.8);
  display: flex;
  justify-content: center;
  align-items: center;
  z-index: 1000;
}

.modal-content {
  background: white;
  border-radius: 8px;
  padding: 20px;
  max-width: 90vw;
  max-height: 90vh;
  position: relative;
  overflow: auto;
}

.modal-close {
  position: absolute;
  top: 10px;
  right: 15px;
  background: none;
  border: none;
  font-size: 24px;
  cursor: pointer;
  color: #666;
}

.modal-image {
  max-width: 100%;
  max-height: 70vh;
  object-fit: contain;
  display: block;
  margin: 0 auto;
}

.modal-info {
  margin-top: 15px;
  padding-top: 15px;
  border-top: 1px solid #eee;
  color: #666;
}

.modal-info p {
  margin: 5px 0;
}

/* 响应式设计 */
@media (max-width: 768px) {
  .gallery-container {
    padding: 15px;
  }
  
  .time-filters {
    flex-direction: column;
    gap: 10px;
  }
  
  .images-grid {
    grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
    gap: 15px;
  }
  
  .image-thumbnail {
    height: 150px;
  }
  
  .pagination {
    gap: 8px;
  }
  
  .page-size-selector {
    width: 100%;
    justify-content: center;
    margin-top: 10px;
  }
}
</style>