<template>
  <div class="home">
    <h1>欢迎来到摄像头系统主页</h1>
    <p>请选择你要访问的功能：</p>

    <div class="button-group">
      <button @click="goTo('stream')">📷 实时摄像头</button>
      <button @click="goTo('album')">🖼️ 查看相册</button>
      <button @click="goTo('upload')">⬆️ 拍照上传</button>
      <button @click="goTo('database')">📁 数据库记录</button>
    </div>
  </div>
</template>

<script setup>
import { useRouter } from 'vue-router';

const router = useRouter();

function goTo(routeName) {
  router.push({ name: routeName });
}
</script>

<style scoped>
.home {
  text-align: center;
  padding: 40px;
}

.button-group {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 16px;
  margin-top: 30px;
}

button {
  padding: 10px 20px;
  font-size: 16px;
  cursor: pointer;
}
</style>
