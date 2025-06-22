<template>
  <view class="gallery-container">
    <text class="title">📷 查看相册</text>

    <!-- 搜索筛选区域 -->
    <view class="search-panel">
      <view class="time-filters">
        <view class="filter-group">
          <text class="label">开始时间：</text>
          <picker mode="date" :value="startTimeDate" @change="onStartDateChange">
            <view class="picker">{{ startTimeDate || '请选择' }}</view>
          </picker>
          <picker mode="time" :value="startTimeTime" @change="onStartTimeChange">
            <view class="picker">{{ startTimeTime || '请选择' }}</view>
          </picker>
        </view>

        <view class="filter-group">
          <text class="label">结束时间：</text>
          <picker mode="date" :value="endTimeDate" @change="onEndDateChange">
            <view class="picker">{{ endTimeDate || '请选择' }}</view>
          </picker>
          <picker mode="time" :value="endTimeTime" @change="onEndTimeChange">
            <view class="picker">{{ endTimeTime || '请选择' }}</view>
          </picker>
        </view>
      </view>

      <view class="quick-actions">
        <button class="btn" @click="setTimeRange('today')">今天</button>
        <button class="btn" @click="setTimeRange('yesterday')">昨天</button>
        <button class="btn" @click="setTimeRange('week')">最近7天</button>
        <button class="btn" @click="setTimeRange('month')">最近30天</button>
        <button class="btn clear-btn" @click="clearFilter">清除筛选</button>
      </view>

      <view class="search-info">
        <text class="result-count">
          {{ loading ? '搜索中...' : `找到 ${totalImages} 张图片` }} {{ hasFilter ? '（已筛选）' : '' }}
        </text>
      </view>
    </view>

    <!-- 调试信息 -->
    <view class="debug-info" v-if="showDebug">
      <text>调试信息：</text>
      <text>图片数量: {{ images.length }}</text>
      <text v-if="images.length > 0">第一张图片数据长度: {{ images[0].image ? images[0].image.length : 0 }}</text>
    </view>

    <!-- 加载状态 -->
    <view v-if="loading" class="loading-state">
      <text>🔄 正在加载图片...</text>
    </view>

    <!-- 无图片状态 -->
    <view v-else-if="images.length === 0" class="no-images">
      <text>📷 {{ hasFilter ? '没有找到符合条件的图片' : '暂无图片' }}</text>
      <text v-if="hasFilter">请尝试调整时间范围或清除筛选条件</text>
    </view>

    <!-- 图片网格 -->
    <view v-else class="images-grid">
      <view
        v-for="image in images"
        :key="image.id"
        class="image-card"
        @click="openImageModal(image)"
      >
        <image
          :src="getImageSrc(image)"
          :mode="'aspectFill'"
          class="image-thumbnail"
          @error="handleImageError"
          @load="handleImageLoad"
        />
        <view class="image-info">
          <text class="image-time">{{ formatTime(image.timestamp) }}</text>
        </view>
      </view>
    </view>

    <!-- 分页控制 -->
    <view v-if="!loading && images.length > 0" class="pagination-container">
      <view class="pagination">
        <button @click="goToPage(1)" :disabled="page === 1">首页</button>
        <button @click="prevPage" :disabled="page === 1">上一页</button>
        <text class="page-info">第 {{ page }} 页</text>
        <button @click="nextPage" :disabled="!hasNextPage">下一页</button>

        <view class="page-size-selector">
          <text>每页显示：</text>
          <picker mode="selector" :range="pageSizes" :value="pageSizes.indexOf(pageSize)" @change="changePageSize">
            <view class="picker">{{ pageSize }}张</view>
          </picker>
        </view>
      </view>

      <view class="page-stats">
        显示第 {{ (page - 1) * pageSize + 1 }} - {{ Math.min(page * pageSize, totalImages) }} 张
      </view>
    </view>

    <!-- 图片弹窗 -->
    <view v-if="showModal" class="modal-overlay" @click="closeModal">
      <view class="modal-content" @click.stop>
        <button class="modal-close" @click="closeModal">✕</button>
        <image
          :src="getImageSrc(selectedImage)"
          mode="aspectFit"
          class="modal-image"
        />
        <view class="modal-info">
          <text><strong>拍摄时间：</strong>{{ formatTime(selectedImage.timestamp) }}</text>
          <text><strong>图片ID：</strong>{{ selectedImage.id }}</text>
        </view>
      </view>
    </view>
  </view>
</template>

<script>
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
      startTimeDate: '',
      startTimeTime: '',
      endTimeDate: '',
      endTimeTime: '',
      showModal: false,
      selectedImage: null,
      pageSizes: [6, 10, 20],
      showDebug: false, // 调试开关
    }
  },

  computed: {
    hasFilter() {
      return this.startTime || this.endTime
    },
    hasNextPage() {
      return this.images.length === this.pageSize
    }
  },
  
  methods: {
    // 获取图片源地址
    getImageSrc(image) {
      if (!image || !image.image) {
        return '/static/placeholder.png' // 占位图
      }
      
      // 检查是否已经包含 data:image 前缀
      if (image.image.startsWith('data:image')) {
        return image.image
      }
      
      // 检查是否已经包含 /9j 等 base64 开头标识
      if (image.image.startsWith('/9j') || image.image.startsWith('iVBOR') || image.image.startsWith('R0lGOD')) {
        return `data:image/jpeg;base64,${image.image}`
      }
      
      // 其他情况直接添加前缀
      return `data:image/jpeg;base64,${image.image}`
    },

    formatTime(timestamp) {
      const date = new Date(timestamp)
      return date.toLocaleString('zh-CN', {
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit',
      })
    },

    toBackendTimeFormat(localDateTimeStr) {
      if (!localDateTimeStr) return ''
      const dt = new Date(localDateTimeStr)
      if (isNaN(dt)) return ''

      const Y = dt.getFullYear()
      const M = dt.getMonth() + 1
      const D = dt.getDate()
      const h = dt.getHours()
      const m = dt.getMinutes()
      const s = dt.getSeconds()

      return `${Y}/${M}/${D} ${this.pad(h)}:${this.pad(m)}:${this.pad(s)}`
    },

    pad(n) {
      return n < 10 ? '0' + n : n
    },

    fetchImages() {
      this.loading = true
      const params = {
        page: this.page,
        page_size: this.pageSize,
      }
      if (this.startTime) params.start = this.toBackendTimeFormat(this.startTime)
      if (this.endTime) params.end = this.toBackendTimeFormat(this.endTime)

      console.log('请求参数:', params) // 调试日志

      uni.request({
        url: 'http://192.168.97.65:8000/images/search',
        method: 'GET',
        data: params,
        success: (res) => {
          console.log('响应数据:', res) // 调试日志
          
          if (res.statusCode === 200) {
            const data = res.data || {}
            this.images = data.data || []
            this.totalImages = data.total || 0
            
            // 调试信息
            if (this.images.length > 0) {
              console.log('第一张图片数据:', this.images[0])
              console.log('图片数据长度:', this.images[0].image ? this.images[0].image.length : 0)
            }
          } else {
            this.images = []
            this.totalImages = 0
            uni.showToast({ title: '获取图片失败', icon: 'none' })
          }
        },
        fail: (err) => {
          console.error('获取相册图片失败:', err)
          this.images = []
          this.totalImages = 0
          uni.showToast({ title: '请求失败', icon: 'none' })
        },
        complete: () => {
          this.loading = false
        },
      })
    },

    goToPage(pageNum) {
      this.page = pageNum
      this.fetchImages()
    },

    nextPage() {
      if (this.hasNextPage) {
        this.page++
        this.fetchImages()
      }
    },

    prevPage() {
      if (this.page > 1) {
        this.page--
        this.fetchImages()
      }
    },

    changePageSize(e) {
      this.pageSize = this.pageSizes[e.detail.value]
      this.page = 1
      this.fetchImages()
    },

    setTimeRange(type) {
      const now = new Date()
      const today = new Date(now.getFullYear(), now.getMonth(), now.getDate())

      this.endTime = this.toDateTimeLocal(now)
      this.endTimeDate = this.endTime.split('T')[0]
      this.endTimeTime = this.endTime.split('T')[1]

      switch (type) {
        case 'today': {
          this.startTime = this.toDateTimeLocal(today)
          this.startTimeDate = this.startTime.split('T')[0]
          this.startTimeTime = this.startTime.split('T')[1]
          break
        }
        case 'yesterday': {
          const yesterday = new Date(today)
          yesterday.setDate(today.getDate() - 1)
          this.startTime = this.toDateTimeLocal(yesterday)
          this.endTime = this.toDateTimeLocal(today)
          this.startTimeDate = this.startTime.split('T')[0]
          this.startTimeTime = this.startTime.split('T')[1]
          this.endTimeDate = this.endTime.split('T')[0]
          this.endTimeTime = this.endTime.split('T')[1]
          break
        }
        case 'week': {
          const weekAgo = new Date(today)
          weekAgo.setDate(today.getDate() - 7)
          this.startTime = this.toDateTimeLocal(weekAgo)
          this.startTimeDate = this.startTime.split('T')[0]
          this.startTimeTime = this.startTime.split('T')[1]
          break
        }
        case 'month': {
          const monthAgo = new Date(today)
          monthAgo.setDate(today.getDate() - 30)
          this.startTime = this.toDateTimeLocal(monthAgo)
          this.startTimeDate = this.startTime.split('T')[0]
          this.startTimeTime = this.startTime.split('T')[1]
          break
        }
      }

      this.page = 1
      this.fetchImages()
    },

    clearFilter() {
      this.startTime = ''
      this.endTime = ''
      this.startTimeDate = ''
      this.startTimeTime = ''
      this.endTimeDate = ''
      this.endTimeTime = ''
      this.page = 1
      this.fetchImages()
    },

    toDateTimeLocal(dt) {
      const Y = dt.getFullYear()
      const M = (dt.getMonth() + 1).toString().padStart(2, '0')
      const D = dt.getDate().toString().padStart(2, '0')
      const h = dt.getHours().toString().padStart(2, '0')
      const m = dt.getMinutes().toString().padStart(2, '0')
      return `${Y}-${M}-${D}T${h}:${m}`
    },

    openImageModal(image) {
      this.selectedImage = image
      this.showModal = true
    },

    closeModal() {
      this.showModal = false
      this.selectedImage = null
    },

    handleImageError(e) {
      console.error('图片加载失败:', e)
      uni.showToast({ title: '图片加载失败', icon: 'none' })
    },

    handleImageLoad(e) {
      console.log('图片加载成功:', e)
    },

    // picker 绑定的时间处理
    onStartDateChange(e) {
      this.startTimeDate = e.detail.value
      this.updateStartTime()
    },
    onStartTimeChange(e) {
      this.startTimeTime = e.detail.value
      this.updateStartTime()
    },
    onEndDateChange(e) {
      this.endTimeDate = e.detail.value
      this.updateEndTime()
    },
    onEndTimeChange(e) {
      this.endTimeTime = e.detail.value
      this.updateEndTime()
    },
    updateStartTime() {
      if (this.startTimeDate && this.startTimeTime) {
        this.startTime = `${this.startTimeDate}T${this.startTimeTime}`
      }
      this.page = 1
      this.fetchImages()
    },
    updateEndTime() {
      if (this.endTimeDate && this.endTimeTime) {
        this.endTime = `${this.endTimeDate}T${this.endTimeTime}`
      }
      this.page = 1
      this.fetchImages()
    },
  },

  mounted() {
    this.fetchImages()
  },
}
</script>

<style scoped>
.gallery-container {
  padding: 20rpx;
  background-color: #fff;
}

.title {
  font-size: 32rpx;
  font-weight: bold;
  margin-bottom: 20rpx;
  text-align: center;
}

.debug-info {
  background-color: #f0f0f0;
  padding: 10rpx;
  margin-bottom: 20rpx;
  border-radius: 6rpx;
}

.debug-info text {
  display: block;
  font-size: 24rpx;
  color: #666;
  margin-bottom: 5rpx;
}

.search-panel {
  margin-bottom: 20rpx;
}

.time-filters {
  display: flex;
  flex-wrap: wrap;
  justify-content: space-between;
  margin-bottom: 10rpx;
}

.filter-group {
  display: flex;
  align-items: center;
  margin-bottom: 10rpx;
  flex-grow: 1;
  min-width: 45%;
}

.label {
  font-size: 28rpx;
  margin-right: 10rpx;
  width: 90rpx;
}

.picker {
  flex-grow: 1;
  padding: 6rpx 10rpx;
  border: 1rpx solid #ccc;
  border-radius: 6rpx;
  font-size: 26rpx;
  text-align: center;
  color: #333;
  margin-right: 10rpx;
}

.quick-actions {
  margin-top: 10rpx;
  display: flex;
  flex-wrap: wrap;
  gap: 10rpx;
  justify-content: center;
}

.btn {
  background-color: #2c90f6;
  color: white;
  padding: 10rpx 16rpx;
  border-radius: 6rpx;
  font-size: 26rpx;
}

.clear-btn {
  background-color: #999;
}

.search-info {
  margin-top: 10rpx;
  text-align: center;
  font-size: 28rpx;
  color: #666;
}

.loading-state,
.no-images {
  margin-top: 50rpx;
  text-align: center;
  font-size: 28rpx;
  color: #999;
}

.no-images text {
  display: block;
  margin-bottom: 10rpx;
}

.images-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 10rpx;
  justify-content: space-between;
}

.image-card {
  width: 30%;
  border: 1rpx solid #ddd;
  border-radius: 8rpx;
  overflow: hidden;
  margin-bottom: 10rpx;
  cursor: pointer;
  background-color: #f9f9f9;
}

.image-thumbnail {
  width: 100%;
  height: 200rpx;
  display: block;
  background-color: #f0f0f0;
}

.image-info {
  padding: 8rpx;
  background-color: #f7f7f7;
  text-align: center;
}

.image-time {
  font-size: 22rpx;
  color: #666;
}

.pagination-container {
  margin-top: 20rpx;
  text-align: center;
}

.pagination {
  display: flex;
  justify-content: center;
  align-items: center;
  gap: 12rpx;
  flex-wrap: wrap;
}

.pagination button {
  padding: 6rpx 12rpx;
  font-size: 26rpx;
  background-color: #2c90f6;
  color: #fff;
  border: none;
  border-radius: 6rpx;
}

.pagination button:disabled {
  background-color: #aaa;
}

.page-info {
  font-size: 26rpx;
  padding: 0 10rpx;
  color: #333;
}

.page-size-selector {
  display: flex;
  align-items: center;
  gap: 6rpx;
  font-size: 26rpx;
}

.page-stats {
  margin-top: 10rpx;
  font-size: 24rpx;
  color: #666;
}

.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  width: 100vw;
  height: 100vh;
  background-color: rgba(0,0,0,0.6);
  display: flex;
  justify-content: center;
  align-items: center;
  z-index: 1000;
}

.modal-content {
  position: relative;
  background-color: #fff;
  padding: 20rpx;
  border-radius: 8rpx;
  max-width: 90vw;
  max-height: 90vh;
  overflow: auto;
}

.modal-close {
  position: absolute;
  top: 10rpx;
  right: 10rpx;
  font-size: 32rpx;
  background: transparent;
  border: none;
  cursor: pointer;
  z-index: 1001;
}

.modal-image {
  width: 100%;
  max-height: 70vh;
  margin-bottom: 10rpx;
}

.modal-info text {
  display: block;
  margin-bottom: 6rpx;
  font-size: 26rpx;
}
</style>