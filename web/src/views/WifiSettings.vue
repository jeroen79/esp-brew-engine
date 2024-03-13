<script lang="ts" setup>
import { ref, onMounted, onBeforeUnmount, inject } from 'vue';
import { mdiEye, mdiEyeOutline, mdiConnection, mdiHelp } from '@mdi/js';
import WebConn from '@/helpers/webConn';
import { IWifiSettings } from '@/interfaces/IWifiSettings';
import { IWifiNetwork } from '@/interfaces/IWifiNetwork';

const webConn = inject<WebConn>('webConn');

const wifiSettings = ref<IWifiSettings>({ // add default value, vue has issues with null values atm
  ssid: '',
  password: '',
  enableAP: true,
  maxPower: 20,
});

const dialogData = ref<IWifiSettings>({ // add default value, vue has issues with null values atm
  ssid: '',
  password: '',
  enableAP: false,
  maxPower: 0,
});

const wifiNetworks = ref<Array<IWifiNetwork>>([]);

const networkTableHeaders = ref<Array<any>>([
  { title: 'SSID', key: 'ssid', align: 'start' },
  { title: 'RSSI', key: 'rssi', align: 'start' },
  { title: 'Channel', key: 'channel', align: 'start' },
  { title: 'Type', key: 'authMode', align: 'start' },
  { title: 'Actions', key: 'actions', sortable: false },
]);
const connectDialog = ref<boolean>(false);

const alert = ref<string>('');
const alertType = ref<'error' | 'success' | 'warning' | 'info' >('info');

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

const scanForNetworks = async () => {
  const requestData = {
    command: 'ScanWifi',
    data: null,
  };

  alert.value = 'Please be patient, scanning in progress...';
  alertType.value = 'info';

  const apiResult = await webConn?.doPostRequest(requestData);

  alert.value = ''; // clear alert

  if (apiResult === undefined || apiResult.success === false) {
    return;
  }
  wifiNetworks.value = apiResult.data;
};

const showConnectDialog = async (item:IWifiNetwork) => {
  dialogData.value.ssid = item.ssid;
  dialogData.value.enableAP = false;
  connectDialog.value = true;
};

const save = async () => {
  if (wifiSettings.value == null) {
    return;
  }

  if (wifiSettings.value.ssid === '') {
    alert.value = 'SSID cannot be empty!';
    alertType.value = 'warning';
    return;
  }

  alert.value = '';

  const requestData = {
    command: 'SaveWifiSettings',
    data: wifiSettings.value,
  };

  const result = await webConn?.doPostRequest(requestData);
  if (result?.message != null) {
    alert.value = result?.message;
    alertType.value = 'warning';
  }
};

const closeConnectDialog = async () => {
  wifiSettings.value.ssid = dialogData.value.ssid;
  wifiSettings.value.password = dialogData.value.password;
  wifiSettings.value.enableAP = false;
  connectDialog.value = false;
  save();
};

// we clear the config when the mode is changed, this is less confusing
const clearConfig = () => {
  wifiSettings.value.ssid = '';
  wifiSettings.value.password = '';
};

onMounted(() => {
  getData();
});

onBeforeUnmount(() => {

});

</script>

<template>
  <v-container class="pa-6" fluid>
    <v-alert :type="alertType" v-if="alert" closable @click:close="alert = ''">{{alert}}</v-alert>
    <v-form fast-fail @submit.prevent>

      <v-row>
        <v-col cols="12" md="6">
          <v-switch
            v-model="wifiSettings.enableAP"
            label="Access Point Mode"
            color="red"
            @click="clearConfig()"
          >
            <template v-slot:append>
              <v-tooltip text="When enabled a wifi network is created by ESP-Brew-Engine, to connect to your own network turn this off!">
                <template v-slot:activator="{ props }">
                  <v-icon size="small" v-bind="props">{{mdiHelp}}</v-icon>
                </template>
              </v-tooltip>
            </template>
          </v-switch>
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="6" v-if="!wifiSettings.enableAP">
          <v-text-field v-model="wifiSettings.ssid" label="Join Network (SSID)" />
        </v-col>
        <v-col cols="12" md="6" v-if="wifiSettings.enableAP">
          <v-text-field v-model="wifiSettings.ssid" label="Access Point Name (SSID)" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12" md="6">
          <v-text-field v-model="wifiSettings.password" :type="hidePwd ? 'password' : 'text'" label="Password" :append-icon="hidePwd ? mdiEye : mdiEyeOutline " @click:append="() => (hidePwd = !hidePwd)" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12" md="6">
          <v-slider
            v-model="wifiSettings.maxPower"
            label="Max Wifi Power (dBm)"
            step="1"
            thumb-label="always"
            max=20
            min=10
          />
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="12">

          <v-form fast-fail @submit.prevent>
            <v-data-table
              :headers="networkTableHeaders"
              :items="wifiNetworks"
              density="compact"
              item-value="name"
            >
              <template v-slot:top>
                <v-toolbar density="compact">
                  <v-toolbar-title>Found Networks</v-toolbar-title>
                  <v-spacer />
                  <v-btn color="secondary" variant="outlined" class="mr-5" @click="scanForNetworks()">
                    Scan
                  </v-btn>

                  <v-dialog v-model="connectDialog" max-width="500px">
                    <v-card>
                      <v-card-title>
                        <span class="text-h5">Connect</span>
                      </v-card-title>

                      <v-card-text>
                        <v-container>
                          <v-row>
                            <v-text-field v-model="dialogData.ssid" label="SSID" readonly />
                          </v-row>
                          <v-row>
                            <v-text-field v-model="dialogData.password" label="Password" />
                          </v-row>
                        </v-container>
                      </v-card-text>

                      <v-card-actions>
                        <v-spacer />
                        <v-btn color="blue-darken-1" variant="text" @click="closeConnectDialog">
                          Connect
                        </v-btn>
                      </v-card-actions>
                    </v-card>
                  </v-dialog>

                </v-toolbar>
              </template>
              <template v-slot:[`item.actions`]="{ item }">
                <v-icon size="small" class="me-2" @click="showConnectDialog(item)" :icon="mdiConnection" />
              </template>
            </v-data-table>

          </v-form>
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
