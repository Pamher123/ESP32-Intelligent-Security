<template>
  <view class="container">
    <view class="title">欢迎使用我的App</view>
    <button @click="openCamera">实时摄像头</button>
    <button @click="goToSensorData">数据记录</button>
    <button @click="goToRphoto">相册</button>
  </view>
</template>

<script>
export default {
  methods: {
    openCamera() {
      uni.navigateTo({
        url: '/pages/camera/camera'
      });
    },
    goToSensorData() {
      uni.navigateTo({
        url: '/pages/sensordata/sensor_data'
      });
    },
    goToRphoto() {
      uni.navigateTo({
        url: '/pages/photo/photoGallery'  // 假设相册是这个页面
      });
    }
  }
}
</script>

<style>
.container {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 100vh;
}
.title {
  font-size: 24px;
  margin-bottom: 40px;
}
button {
  width: 200px;
  margin: 10px;
  padding: 15px;
  background-color: #007aff;
  color: white;
  border-radius: 10px;
}
</style>
