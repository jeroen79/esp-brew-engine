// Composables
import { createRouter, createWebHistory } from 'vue-router';

const routes = [
  {
    path: '/',
    component: () => import('@/layouts/default/View.vue'),
    children: [
      {
        path: '',
        name: 'Home',
        // route level code-splitting
        // this generates a separate chunk (about.[hash].js) for this route
        // which is lazy-loaded when the route is visited.
        component: () => import(/* webpackChunkName: "home" */ '@/views/Control.vue'),
      },
      {
        path: 'control',
        name: 'Control',
        component: () => import(/* webpackChunkName: "Control" */ '@/views/Control.vue'),
      },
      {
        path: 'import',
        name: 'Import',
        component: () => import(/* webpackChunkName: "Import" */ '@/views/Import.vue'),
      },
      {
        path: 'refractometer',
        name: 'Refractometer',
        component: () => import(/* webpackChunkName: "Refractometer" */ '@/views/Refractometer.vue'),
      },
      {
        path: 'mashschedules',
        name: 'MashSchedules',
        component: () => import(/* webpackChunkName: "MashSchedules" */ '@/views/MashSchedules.vue'),
      },
      {
        path: 'pidsettings',
        name: 'PidSettings',
        component: () => import(/* webpackChunkName: "PidSettings" */ '@/views/PidSettings.vue'),
      },
      {
        path: 'tempsettings',
        name: 'TempSettings',
        component: () => import(/* webpackChunkName: "TempSettings" */ '@/views/TempSettings.vue'),
      },
      {
        path: 'wifiSettings',
        name: 'WifiSettings',
        component: () => import(/* webpackChunkName: "WifiSettings" */ '@/views/WifiSettings.vue'),
      },
      {
        path: 'systemSettings',
        name: 'SystemSettings',
        component: () => import(/* webpackChunkName: "SystemSettings" */ '@/views/SystemSettings.vue'),
      },
      {
        path: 'heaterSettings',
        name: 'HeaterSettings',
        component: () => import(/* webpackChunkName: "HeaterSettings" */ '@/views/HeaterSettings.vue'),
      },
      {
        path: 'clientSettings',
        name: 'ClientSettings',
        component: () => import(/* webpackChunkName: "ClientSettings" */ '@/views/ClientSettings.vue'),
      },
    ],
  },
];

const router = createRouter({
  history: createWebHistory(process.env.BASE_URL),
  routes,
});

export default router;
