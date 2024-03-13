<script lang="ts" setup>
import { mdiDelete, mdiPencil } from '@mdi/js';
import { inject, onBeforeUnmount, onMounted, ref } from 'vue';
import { IHeater } from '@/interfaces/IHeater';
import WebConn from '@/helpers/webConn';

const webConn = inject<WebConn>('webConn');

const HeaterConfigs = ref<Array<IHeater>>([]);

const tableHeaders = ref<Array<any>>([
  { title: 'Name', key: 'name', align: 'start' },
  { title: 'Pin', key: 'pinNr', align: 'start' },
  { title: 'Preference', key: 'preference', align: 'start' },
  { title: 'Power (Watt)', key: 'watt', align: 'start' },
  { title: 'Use for Mash', key: 'useForMash', align: 'end' },
  { title: 'Use for Boil', key: 'useForBoil', align: 'end' },
  { title: 'Actions', key: 'actions', align: 'end', sortable: false },
]);

const dialog = ref<boolean>(false);
const dialogDelete = ref<boolean>(false);

const alert = ref<string>('');
const alertType = ref<'error' | 'success' | 'warning' | 'info' >('info');

const defaultHeater:IHeater = {
  id: 0,
  name: 'New Heater',
  pinNr: 0,
  preference: 0,
  watt: 0,
  useForMash: true,
  useForBoil: true,
};

const editedItem = ref<IHeater>(defaultHeater);

const getData = async () => {
  const requestData = {
    command: 'GetHeaterSettings',
    data: null,
  };

  const apiResult = await webConn?.doPostRequest(requestData);

  if (apiResult === undefined || apiResult.success === false) {
    return;
  }

  HeaterConfigs.value = apiResult.data;
};

const newItem = async () => {
  const newHeater = { ...defaultHeater };
  newHeater.id = Math.max(...HeaterConfigs.value.map((s) => s.id)) + 1;
  HeaterConfigs.value.push(newHeater);

  editedItem.value = newHeater;
  dialog.value = true;
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

const editItem = async (item:IHeater) => {
  editedItem.value = item;
  dialog.value = true;
};

const openDeleteDialog = async (item:IHeater) => {
  editedItem.value = item;
  dialogDelete.value = true;
};

const deleteItemOk = async () => {
  // filter out deleted
  HeaterConfigs.value = HeaterConfigs.value.filter((s) => s.id !== editedItem.value.id);
  closeDeleteDialog();
};

const save = async () => {
  if (HeaterConfigs.value == null) {
    return;
  }

  const requestData = {
    command: 'SaveHeaterSettings',
    data: HeaterConfigs.value,
  };

  const result = await webConn?.doPostRequest(requestData);

  if (result?.message != null) {
    alertType.value = 'warning';
    alert.value = result?.message;
  }
};

</script>

<template>
  <v-container class="pa-6" fluid>
    <v-alert :type="alertType" v-if="alert" closable @click:close="alert = ''">{{alert}}</v-alert>
    <v-form fast-fail @submit.prevent>
      <v-data-table
        :headers="tableHeaders"
        :items="HeaterConfigs"
        density="compact"
        item-value="name"
      >
        <template v-slot:top>
          <v-toolbar density="compact">
            <v-toolbar-title>Heater Configurations</v-toolbar-title>
            <v-spacer />
            <v-btn color="secondary" variant="outlined" class="mr-5" @click="newItem()">
              New Heater
            </v-btn>
            <v-btn color="secondary" variant="outlined" class="mr-5" @click="getData()">
              Refresh
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
                      <v-text-field v-model.number="editedItem.pinNr" label="Gpio Pin Nr" />
                    </v-row>
                    <v-row>
                      <v-text-field v-model.number="editedItem.preference" label="Preference" />
                    </v-row>
                    <v-row>
                      <v-text-field v-model.number="editedItem.watt" label="Power (Watt)" />
                    </v-row>
                    <v-row>
                      <v-switch v-model="editedItem.useForMash" label="Use for Mash" color="orange" />
                    </v-row>
                    <v-row>
                      <v-switch v-model="editedItem.useForBoil" label="Use for Boil" color="red" />
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
          <v-icon size="small" class="me-2" @click="editItem(item)" :icon="mdiPencil" />
          <v-icon size="small" @click="openDeleteDialog(item)" :icon="mdiDelete" />
        </template>
        <template v-slot:[`item.useForMash`]="{ item }">
          <v-checkbox-btn class="align-right justify-center" v-model="item.useForMash" disabled />
        </template>
        <template v-slot:[`item.useForBoil`]="{ item }">
          <v-checkbox-btn class="align-right justify-center" v-model="item.useForBoil" disabled />
        </template>
      </v-data-table>

      <v-btn color="success" class="mt-4 mr-2" @click="save"> Save </v-btn>

    </v-form>
  </v-container>
</template>
