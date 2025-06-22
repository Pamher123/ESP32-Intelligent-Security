<template>
  <view class="container">
    <text class="title">实时摄像头</text>
    <image
      :src="streamUrl"
      mode="widthFix"
      v-if="streamUrl"
      class="stream"
    />
    <text v-else class="error">无法连接到摄像头流</text>

    <!-- 底部固定按钮 -->
    <view class="bottom-bar">
      <view class="btn" @click="takePhoto">拍照</view>
    </view>
  </view>
</template>

<script setup>
import { ref, onMounted } from 'vue'

const streamUrl = ref('')

onMounted(() => {
  streamUrl.value = 'http://192.168.97.65:8000/stream' // 你的实时流地址
})

function takePhoto() {
  uni.request({
    url: 'http://192.168.97.18/take-photo',
    method: 'GET',
    timeout: 5000,
    success(res) {
      if (res.statusCode === 200) {
        uni.showToast({ title: '拍照请求成功', icon: 'success' })
        console.log('📸 拍照成功，状态码:', res.statusCode)
      } else {
        uni.showToast({ title: '拍照请求失败', icon: 'error' })
        console.error('❌ 拍照失败，状态码:', res.statusCode)
      }
    },
    fail(err) {
      uni.showToast({ title: '请求出错', icon: 'error' })
      console.error('❌ 请求失败:', err)
    }
  })
}
</script>

<style>
.container {
  display: flex;
  flex-direction: column;
  height: 100vh; /* 让容器撑满全屏高 */
  padding: 20rpx;
  box-sizing: border-box;
  position: relative; /* 让底部按钮绝对定位生效 */
}

.title {
  font-size: 40rpx;
  margin-bottom: 20rpx;
  text-align: center;
}

.stream {
  width: 100%;
  height: 400rpx; /* 固定高度 */
  object-fit: contain;
  border: 1rpx solid #ccc;
  border-radius: 10rpx;
  margin-bottom: 20rpx;
}

.error {
  color: red;
  margin-bottom: 20rpx;
  text-align: center;
  height: 400rpx; /* 和stream同高，保持布局 */
  line-height: 400rpx;
}

.bottom-bar {
  position: absolute;
  bottom: 20rpx;
  width: 100%;
  display: flex;
  justify-content: center;
}

.btn {
  width: 200rpx;
  height: 60rpx;
  background-color: #007aff;
  color: white;
  font-size: 32rpx;
  border-radius: 10rpx;
  text-align: center;
  line-height: 60rpx;
  user-select: none;
}
</style>
