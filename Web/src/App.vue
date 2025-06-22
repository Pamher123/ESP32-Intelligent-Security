<template>
  <div id="app">
    <!-- 导航栏（可选） -->
    <nav>
      <router-link to="/">首页</router-link> |
      <router-link to="/stream">实时摄像头</router-link> |
      <router-link to="/album">相册</router-link> |
      <router-link to="/upload">拍照上传</router-link> |
      <router-link to="/database">数据库</router-link>
    </nav>
    
    <!-- 这个很重要！路由组件会在这里渲染 -->
    <router-view />
  </div>
</template>

<script>
export default {
  name: 'App'
}
</script>

<style>
#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  color: #2c3e50;
}

nav {
  padding: 30px;
  text-align: center;
}

nav a {
  font-weight: bold;
  color: #2c3e50;
  text-decoration: none;
  margin: 0 10px;
}

nav a.router-link-exact-active {
  color: #42b983;
}
</style>