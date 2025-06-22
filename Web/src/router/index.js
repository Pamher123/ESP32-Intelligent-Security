import { createRouter, createWebHistory } from 'vue-router';
import HomeView from '../views/HomeView.vue';
import PhotoGallery from '../views/PhotoGallery.vue';
import UploadSnapshot from '../views/UploadSnapshot.vue';
import SensorData from '../views/SensorData.vue';
import CameraStream from '../views/CameraStream.vue';

const routes = [
  {
    path: '/',
    name: 'home',
    component: HomeView
  },
  {
    path: '/stream',
    name: 'stream',
    component: CameraStream
  },
  {
    path: '/album',
    name: 'album',
    component: PhotoGallery
  },
  {
    path: '/upload',
    name: 'upload',
    component: UploadSnapshot
  },
  {
    path: '/database',
    name: 'database',
    component: SensorData
  }
];

const router = createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes
});

export default router;