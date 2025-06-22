<template>
  <div>
    <h2>拍照上传</h2>
    <button @click="triggerEsp32">拍照并上传</button>
    <p v-if="status">{{ status }}</p>
  </div>
</template>

<script setup>
import axios from 'axios';
import { ref } from 'vue';

const status = ref('');

const triggerEsp32 = async () => {
  status.value = '请求ESP32拍照中...';
  try {
    // 注意：这是 ESP32 的地址
    await axios.get('http://192.168.97.18/take-photo'); // 假设 ESP32 是这个IP
    status.value = 'ESP32 正在上传...';
  } catch (err) {
    status.value = '请求ESP32成功';
  }
};
</script>
