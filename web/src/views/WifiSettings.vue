<script lang="ts" setup>
import { ref, onMounted, onBeforeUnmount, watch, inject, computed, reactive, watchEffect } from 'vue';
import { mdiEye, mdiEyeOutline } from '@mdi/js';
import WebConn from '@/helpers/webConn';
import { IWifiSettings } from '@/interfaces/IWifiSettings';

const webConn = inject<WebConn>('webConn');

const wifiSettings = ref<IWifiSettings>({ // add default value, vue has issues with null values atm
  ssid: '',
  password: '',
  enableAP: true,
});

const alert = ref<string>('');

const hidePwd = ref<boolean>(true);

const getData = async () => {
  const requestData = {
    command: 'GetWifiSettings',
    data: null,
  };

  const apiResult = await webConn?.doPostRequest(requestData);

  if (apiResult === undefined || apiResult.success === false) {
    return;
  }
  wifiSettings.value = apiResult.data;
};

onMounted(() => {
  getData();
});

onBeforeUnmount(() => {

});

const save = async () => {
  if (wifiSettings.value == null) {
    return;
  }

  const requestData = {
    command: 'SaveWifiSettings',
    data: wifiSettings.value,
  };

  const result = await webConn?.doPostRequest(requestData);
  if (result?.message != null) {
    alert.value = result?.message;
  }
};

</script>

<template>
  <v-container class="spacing-playground pa-6" fluid>
    <v-alert type="warning" v-if="alert">{{alert}}</v-alert>
    <v-form fast-fail @submit.prevent>

      <v-row>
        <v-col cols="12" md="3">
          <v-text-field v-model="wifiSettings.ssid" label="SSID" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12" md="3">
          <v-text-field v-model="wifiSettings.password" :type="hidePwd ? 'password' : 'text'" label="Password" :append-icon="hidePwd ? mdiEye : mdiEyeOutline " @click:append="() => (hidePwd = !hidePwd)" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12" md="3">
          <v-checkbox v-model="wifiSettings.enableAP" label="Access Point Mode" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12" md="3">
          <v-btn color="success" class="mt-4 mr-2" @click="save"> Save </v-btn>
        </v-col>
      </v-row>
    </v-form>
  </v-container>
</template>
