<script lang="ts" setup>
import WebConn from "@/helpers/webConn";
import { IPidSettings } from "@/interfaces/IPidSettings";
import { mdiHelp } from "@mdi/js";
import { inject, onBeforeUnmount, onMounted, ref } from "vue";

const webConn = inject<WebConn>("webConn");

const pidSettings = ref<IPidSettings>({
  // add default value, vue heeft issues met default null waardes
  kP: 0,
  kI: 0,
  kD: 0,
  boilkP: 0,
  boilkI: 0,
  boilkD: 0,
  pidLoopTime: 60,
  stepInterval: 60,
  boostModeUntil: 85,
});

const getData = async () => {
  const requestData = {
    command: "GetPIDSettings",
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

onBeforeUnmount(() => {});

const save = async () => {
  if (pidSettings.value == null) {
    return;
  }

  // force looptime to be int
  pidSettings.value.pidLoopTime = Math.floor(pidSettings.value.pidLoopTime);

  const requestData = {
    command: "SavePIDSettings",
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
          <v-text-field type="number" v-model.number="pidSettings.pidLoopTime" :label="$t('pidSettings.pidLoopTime')">
            <template v-slot:append>
              <v-tooltip :text="$t('pidSettings.pidLoopTime_tooltip')">
                <template v-slot:activator="{ props }">
                  <v-icon size="small" v-bind="props">{{ mdiHelp }}</v-icon>
                </template>
              </v-tooltip>
            </template>
          </v-text-field>
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="3">
          <v-text-field type="number" v-model.number="pidSettings.stepInterval" :label="$t('pidSettings.stepInterval')">
            <template v-slot:append>
              <v-tooltip :text="$t('pidSettings.stepInterval_tooltip')">
                <template v-slot:activator="{ props }">
                  <v-icon size="small" v-bind="props">{{ mdiHelp }}</v-icon>
                </template>
              </v-tooltip>
            </template>
          </v-text-field>
        </v-col>
      </v-row>

      <div class="text-subtitle-2 mt-4 mb-2">{{ $t('pidSettings.mash') }}</div>

      <v-divider :thickness="7" />

      <v-row class="mt-4 mb-2">
        <v-col cols="12" md="3">
          <v-text-field type="number" v-model.number="pidSettings.kP" label="P" />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field type="number" v-model.number="pidSettings.kI" label="I" />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field type="number" v-model.number="pidSettings.kD" label="D" />
        </v-col>
      </v-row>

      <div class="text-subtitle-2 mt-4 mb-2">{{ $t('pidSettings.boil') }}</div>

      <v-divider :thickness="7" />

      <v-row class="mt-4 mb-2">
        <v-col cols="12" md="3">
          <v-text-field type="number" v-model.number="pidSettings.boilkP" label="P" />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field type="number" v-model.number="pidSettings.boilkI" label="I" />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field type="number" v-model.number="pidSettings.boilkD" label="D" />
        </v-col>
      </v-row>

      <div class="text-subtitle-2 mt-4 mb-2">{{ $t('pidSettings.boost') }}</div>

      <v-row class="mt-4 mb-2">
        <v-col cols="12" md="3">
          <v-text-field type="number" v-model.number="pidSettings.boostModeUntil" :label="$t('pidSettings.boost_until')" :min="0" :max="100">
            <template v-slot:append>
              <v-tooltip :text="$t('pidSettings.boost_until_tooltip')">
                <template v-slot:activator="{ props }">
                  <v-icon size="small" v-bind="props">{{ mdiHelp }}</v-icon>
                </template>
              </v-tooltip>
            </template>
            </v-text-field>
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="3">
          <v-btn color="success" class="mt-4 mr-2" @click="save"> {{ $t('general.save') }} </v-btn>
        </v-col>
      </v-row>

    </v-form>
  </v-container>
</template>
