<script lang="ts" setup>

import { useTheme } from 'vuetify/lib/framework.mjs';
import { useClientStore } from './store/client';
import { onMounted, watch } from 'vue';

const clientStore = useClientStore();
const theme = useTheme();

onMounted(() => {
  theme.global.name.value = clientStore.clientSettings.darkMode ? "dark" : "light";
});

watch(() => clientStore.clientSettings.darkMode, () => {
  theme.global.name.value = clientStore.clientSettings.darkMode ? "dark" : "light";
});

</script>

<template>
  <router-view />
</template>
