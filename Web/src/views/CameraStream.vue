<template>
  <div>
    <h2>实时摄像头</h2>
    <img :src="streamUrl" alt="实时流" v-if="streamUrl" style="max-width: 100%;" />
    <p v-else>无法连接实时流</p>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue';

const streamUrl = ref('');

onMounted(() => {
  // 设置视频流地址
  streamUrl.value = 'http://192.168.97.65:8000/stream'; // 替换为真实IP
});
</script>
