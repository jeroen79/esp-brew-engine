<script lang="ts" setup>
import { inject, onBeforeUnmount, onMounted, ref } from 'vue';
import WebConn from '@/helpers/webConn';
import { IPidSettings } from '@/interfaces/IPidSettings';

const webConn = inject<WebConn>('webConn');

const pidSettings = ref<IPidSettings>({ // add default value, vue heeft issues met default null waardes
  kP: 0,
  kI: 0,
  kD: 0,
  pidLoopTime: 60,
});

const getData = async () => {
  const requestData = {
    command: 'GetPIDSettings',
    data: null,
  };

  const apiResult = await webConn?.doPostRequest(requestData);

  if (apiResult === undefined || apiResult.success === false) {
    return;
  }
  pidSettings.value = apiResult.data;
};

onMounted(() => {
  getData();
});

onBeforeUnmount(() => {

});

const save = async () => {
  if (pidSettings.value == null) {
    return;
  }

  // force looptime to be int
  pidSettings.value.pidLoopTime = Math.floor(pidSettings.value.pidLoopTime);

  const requestData = {
    command: 'SavePIDSettings',
    data: pidSettings.value,
  };

  await webConn?.doPostRequest(requestData);
  // todo capture result and log errors
};

</script>

<template>
  <v-container class="spacing-playground pa-6" fluid>
    <v-form fast-fail @submit.prevent>

      <v-row>
        <v-col cols="12" md="3">
          <v-text-field type="number" v-model.number="pidSettings.kP" label="P" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12" md="3">
          <v-text-field type="number" v-model.number="pidSettings.kI" label="I" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12" md="3">
          <v-text-field type="number" v-model.number="pidSettings.kD" label="D" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12" md="3">
          <v-text-field type="number" v-model.number="pidSettings.pidLoopTime" label="PID Loop Time" />
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
