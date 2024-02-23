<script lang="ts" setup>
import { mdiHelp } from '@mdi/js';
import { inject, onBeforeUnmount, onMounted, ref } from 'vue';
import WebConn from '@/helpers/webConn';
import { ISystemSettings } from '@/interfaces/ISystemSettings';

const webConn = inject<WebConn>('webConn');

const systemSettings = ref<ISystemSettings>({ // add default value, vue has issues with null values atm
  onewirePin: 0,
  stirPin: 0,
  invertOutputs: false,
  mqttUri: '',
  temperatureScale: 0,
});

// is same as enum TemperatureScale, but this wel never change, converting enum to options would be wastefull
const temperatureScales = [
  { title: 'Celsius', value: 0 },
  { title: 'Fahrenheit', value: 1 },
];

const alert = ref<string>('');
const alertType = ref<'error' | 'success' | 'warning' | 'info' >('info');

const dialogFactoryReset = ref<boolean>(false);

const getData = async () => {
  const requestData = {
    command: 'GetSystemSettings',
    data: null,
  };

  const apiResult = await webConn?.doPostRequest(requestData);

  if (apiResult === undefined || apiResult.success === false) {
    return;
  }
  systemSettings.value = apiResult.data;
};

onMounted(() => {
  getData();
});

onBeforeUnmount(() => {

});

const save = async () => {
  if (systemSettings.value == null) {
    return;
  }

  const requestData = {
    command: 'SaveSystemSettings',
    data: systemSettings.value,
  };

  const result = await webConn?.doPostRequest(requestData);
  if (result?.message != null) {
    alert.value = result?.message;
  }
};

const recovery = async () => {
  const requestData = {
    command: 'BootIntoRecovery',
  };

  const result = await webConn?.doPostRequest(requestData);
  if (result?.message != null) {
    alertType.value = 'warning';
    alert.value = result?.message;
  }

  if (result?.success) {
    setTimeout(() => {
      document.location.href = '/';
    }, 10000);
  }
};

const reboot = async () => {
  const requestData = {
    command: 'Reboot',
  };

  const result = await webConn?.doPostRequest(requestData);
  if (result?.message != null) {
    alertType.value = 'warning';
    alert.value = result?.message;
  }

  if (result?.success) {
    setTimeout(() => {
      document.location.href = '/';
    }, 10000);
  }
};

const factoryReset = async () => {
  const requestData = {
    command: 'FactoryReset',
  };

  dialogFactoryReset.value = false;

  const result = await webConn?.doPostRequest(requestData);
  if (result?.message != null) {
    alertType.value = 'warning';
    alert.value = result?.message;
  }

  if (result?.success) {
    setTimeout(() => {
      document.location.href = '/';
    }, 10000);
  }
};

const scaleChanged = () => {
  alertType.value = 'info';
  alert.value = 'Mash Schedules are not automaticly converted, please update then manualy!';
};

</script>

<template>
  <v-container class="spacing-playground pa-6" fluid>
    <v-alert :type="alertType" v-if="alert">{{alert}}</v-alert>
    <v-form fast-fail @submit.prevent>

      <v-row>
        <v-col cols="12" md="3">
          <v-text-field requierd v-model.number="systemSettings.onewirePin" label="Onewire Pin Nr">
            <template v-slot:append>
              <v-tooltip text="IO Number for OneWire">
                <template v-slot:activator="{ props }">
                  <v-icon size="small" v-bind="props">{{mdiHelp}}</v-icon>
                </template>
              </v-tooltip>
            </template>
          </v-text-field>
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="3">
          <v-text-field v-model.number="systemSettings.stirPin" label="Stir/Pump Pin Nr">
            <template v-slot:append>
              <v-tooltip text="IO Number for Stir/Pump (Optional), Set to 0 to disable">
                <template v-slot:activator="{ props }">
                  <v-icon size="small" v-bind="props">{{mdiHelp}}</v-icon>
                </template>
              </v-tooltip>
            </template>
          </v-text-field>
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="3">
          <v-checkbox v-model="systemSettings.invertOutputs" label="Invert Outputs">
            <template v-slot:append>
              <v-tooltip text="When enabled output logic is reverserd 0 <=> 1 this might me needed for some boards">
                <template v-slot:activator="{ props }">
                  <v-icon size="small" v-bind="props">{{mdiHelp}}</v-icon>
                </template>
              </v-tooltip>
            </template>
          </v-checkbox>
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="3">
          <v-text-field v-model="systemSettings.mqttUri" placeholder="mqtt://user:password@192.168.0.1:1883" label="MQTT Uri">
            <template v-slot:append>
              <v-tooltip text="Mqtt Uri, this must start with mqtt:// and can contail an optional username/password. if you use a ipv6 address ->  mqtt://user:password@[0001:0001:0001:0001:0001:0001:0001:0001]:1883">
                <template v-slot:activator="{ props }">
                  <v-icon size="small" v-bind="props">{{mdiHelp}}</v-icon>
                </template>
              </v-tooltip>
            </template>
          </v-text-field>
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="3">
          <v-select label="Temperature Scale" v-model="systemSettings.temperatureScale" :items="temperatureScales" @blur="scaleChanged" />
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="3">
          <v-btn color="success" class="mt-4 mr-2" @click="save"> Save </v-btn>
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="12" />
      </v-row>

      <v-row>
        <v-col cols="3" md="3">
          <v-btn name="reboot" color="warning" variant="outlined" class="mt-4 mr-2" @click="reboot"> Reboot </v-btn>
        </v-col>
        <v-col cols="3" md="3">
          <v-btn name="factoryreset" color="error" variant="outlined" class="mt-4 mr-2" @click="dialogFactoryReset = true"> Factory Reset </v-btn>

          <v-dialog v-model="dialogFactoryReset" max-width="500px">
            <v-card>
              <v-card-title class="text-h5">Are you sure?</v-card-title>
              <v-card-text>Are you sure you want te reset all device settings? <br /> This will also include wifi settings, you will have to reconnect!</v-card-text>
              <v-card-actions>
                <v-spacer />
                <v-btn variant="outlined" @click="dialogFactoryReset = false">No</v-btn>
                <v-btn variant="outlined" color="red" @click="factoryReset">Yes Wipe All Data</v-btn>
                <v-spacer />
              </v-card-actions>
            </v-card>
          </v-dialog>
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="12">
          <label for="recovery">To update firmware the device first needs to be booted into recovery mode</label>
          <br />
          <v-btn name="recovery" color="warning" variant="outlined" class="mt-4 mr-2" @click="recovery"> Boot into recovery </v-btn>
        </v-col>
      </v-row>
    </v-form>
  </v-container>
</template>
