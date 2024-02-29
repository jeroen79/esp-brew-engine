<script lang="ts" setup>
import { ref, onMounted, onBeforeUnmount, inject } from 'vue';
import { VDataTable } from 'vuetify/labs/VDataTable';
import { mdiPencil, mdiDelete, mdiPalette } from '@mdi/js';
import WebConn from '@/helpers/webConn';
import { ITempSensor } from '@/interfaces/ITempSensor';

const webConn = inject<WebConn>('webConn');

const tempSensors = ref<Array<ITempSensor>>([]);

const tableHeaders = ref<Array<any>>([
  { title: 'Id', key: 'id', align: 'start' },
  { title: 'Name', key: 'name', align: 'start' },
  { title: 'Color', key: 'color', align: 'start' },
  { title: 'Compensate Absolute (+-)', key: 'compensateAbsolute', align: 'start' },
  { title: 'Compensate Relative (*)', key: 'compensateRelative', align: 'start' },
  { title: 'Show', key: 'show', align: 'end' },
  { title: 'Use for Control', key: 'useForControl', align: 'end' },
  { title: 'Connected', key: 'connected', align: 'end' },
  { title: 'Last Temp', key: 'lastTemp', align: 'end' },
  { title: 'Actions', key: 'actions', align: 'end', sortable: false },
]);

const dialog = ref<boolean>(false);
const dialogDelete = ref<boolean>(false);

const alert = ref<string>('');
const alertType = ref<'error' | 'success' | 'warning' | 'info' >('info');

const defaultSensor:ITempSensor = {
  id: '',
  name: 'New Sensor',
  color: '#ffffff',
  useForControl: false,
  show: false,
  connected: false,
  compensateAbsolute: 0.0,
  compensateRelative: 1,
  lastTemp: 0,
};

const editedItem = ref<ITempSensor>(defaultSensor);

const getData = async () => {
  const requestData = {
    command: 'GetTempSettings',
    data: null,
  };

  const apiResult = await webConn?.doPostRequest(requestData);

  if (apiResult === undefined || apiResult.success === false) {
    return;
  }

  tempSensors.value = apiResult.data;
};

const detectTempSensors = async () => {
  const requestData = {
    command: 'DetectTempSensors',
    data: null,
  };

  alert.value = 'Please be patient, scanning in progress...';
  alertType.value = 'info';

  const apiResult = await webConn?.doPostRequest(requestData);

  alert.value = ''; // clear alert

  if (apiResult === undefined || apiResult.success === false) {
    return;
  }

  // after successfull detect get new data
  getData();
};

onMounted(() => {
  getData();
});

onBeforeUnmount(() => {

});

const closeDialog = async () => {
  dialog.value = false;
};

const closeDeleteDialog = async () => {
  dialogDelete.value = false;
};

const editItem = async (item:ITempSensor) => {
  editedItem.value = item;
  dialog.value = true;
};

const openDeleteDialog = async (item:ITempSensor) => {
  editedItem.value = item;
  dialogDelete.value = true;
};

const deleteItemOk = async () => {
  // filter out deleted
  tempSensors.value = tempSensors.value.filter((s) => s.id !== editedItem.value.id);
  closeDeleteDialog();
};

const save = async () => {
  if (tempSensors.value == null) {
    return;
  }

  const requestData = {
    command: 'SaveTempSettings',
    data: tempSensors.value,
  };

  await webConn?.doPostRequest(requestData);
  // todo capture result and log errors
};

</script>

<template>
  <v-container class="pa-6" fluid>
    <v-alert :type="alertType" v-if="alert" closable @click:close="alert = ''">{{alert}}</v-alert>
    <v-form fast-fail @submit.prevent>
      <v-data-table
        :headers="tableHeaders"
        :items="tempSensors"
        density="compact"
        item-value="name"
      >
        <template v-slot:top>
          <v-toolbar density="compact">
            <v-toolbar-title>Temp Sensors</v-toolbar-title>
            <v-spacer />
            <v-btn color="secondary" variant="outlined" class="mr-5" @click="getData()">
              Refresh
            </v-btn>
            <v-btn color="secondary" variant="outlined" class="mr-5" @click="detectTempSensors()">
              Detect
            </v-btn>

            <v-dialog v-model="dialog" max-width="500px">
              <v-card>
                <v-card-title>
                  <span class="text-h5">Edit</span>
                </v-card-title>

                <v-card-text>
                  <v-container>
                    <v-row>
                      <v-text-field v-model="editedItem.name" label="Name" />
                    </v-row>
                    <v-row>
                      <v-switch v-model="editedItem.show" label="Show" color="green" />
                    </v-row>
                    <v-row>
                      <v-switch v-model="editedItem.useForControl" label="Enabled" color="red" />
                    </v-row>
                    <v-row>
                      <v-color-picker v-model="editedItem.color" hide-inputs label="Color" />
                    </v-row>
                    <v-row>
                      <v-text-field type="number" v-model.number="editedItem.compensateAbsolute" label="Compensate Absolute (+-)" />
                    </v-row>
                    <v-row>
                      <v-text-field type="number" v-model.number="editedItem.compensateRelative" label="Compensate Relative (*)" />
                    </v-row>
                  </v-container>
                </v-card-text>

                <v-card-actions>
                  <v-spacer />
                  <v-btn color="blue-darken-1" variant="text" @click="closeDialog">
                    Close
                  </v-btn>
                </v-card-actions>
              </v-card>
            </v-dialog>
            <v-dialog v-model="dialogDelete" max-width="500px">
              <v-card>
                <v-card-title class="text-h5">Are you sure you want to delete this item?</v-card-title>
                <v-card-actions>
                  <v-spacer />
                  <v-btn color="blue-darken-1" variant="text" @click="closeDeleteDialog">Cancel</v-btn>
                  <v-btn color="blue-darken-1" variant="text" @click="deleteItemOk">OK</v-btn>
                  <v-spacer />
                </v-card-actions>
              </v-card>
            </v-dialog>
          </v-toolbar>
        </template>
        <template v-slot:[`item.actions`]="{ item }">
          <v-icon size="small" class="me-2" @click="editItem(item.raw)" :icon="mdiPencil" />
          <v-icon size="small" @click="openDeleteDialog(item.raw)" :icon="mdiDelete" />
        </template>
        <template v-slot:[`item.useForControl`]="{ item }">
          <v-checkbox-btn class="align-right justify-center" v-model="item.columns.useForControl" disabled />
        </template>
        <template v-slot:[`item.show`]="{ item }">
          <v-checkbox-btn class="align-right justify-center" v-model="item.columns.show" disabled />
        </template>
        <template v-slot:[`item.connected`]="{ item }">
          <v-checkbox-btn class="align-right justify-center" v-model="item.columns.connected" disabled />
        </template>
        <template v-slot:[`item.color`]="{ item }">
          <v-icon size="small" class="me-2" :icon="mdiPalette" :color="item.columns.color" />
        </template>
      </v-data-table>

      <v-btn color="success" class="mt-4 mr-2" @click="save"> Save </v-btn>

    </v-form>
  </v-container>
</template>
