<script lang="ts" setup>
import { INotification, defaultNotification } from "@/interfaces/INotification";
import { mdiDelete, mdiPencil } from "@mdi/js";
import { ref, watch } from "vue";
import { useI18n } from "vue-i18n";
const { t } = useI18n({ useScope: "global" });

const props = defineProps({
  label: { type: String, required: false, default: "Notifications" },
  itemsPerPage: { type: Number, required: false, default: 50 },
  allowNew: { type: Boolean, required: false, default: true },
});

// notifications as 2-way data with v-model
const notifications = defineModel<Array<INotification>>({ required: true });

const itemsPerPage = ref<number>(props.itemsPerPage);
watch(
  () => props.itemsPerPage,
  () => {
    itemsPerPage.value = props.itemsPerPage;
  },
);

const allowNew = ref<boolean>(props.allowNew);
watch(
  () => props.allowNew,
  () => {
    allowNew.value = props.allowNew;
  },
);

// Notifications
const tableNotificationsHeaders = ref<Array<any>>([
  { title: t("notificationEditor.name"), key: "name", align: "start" },
  { title: t("notificationEditor.time_from_start"), key: "timeFromStart", align: "start" },
  { title: t("notificationEditor.buzzer"), key: "buzzer", align: "start" },
  { title: t("notificationEditor.actions"), key: "actions", sortable: false },
]);
const sortNotificationsBy = ref<Array<any>>([{ key: "index", order: "asc" }]);

const showNotificationDialog = ref<boolean>(false);
const showNotificationDeleteDialog = ref<boolean>(false);
const editingNotification = ref<INotification>(defaultNotification);

const closeDialog = async () => {
  showNotificationDialog.value = false;
};

const closeDeleteDialog = async () => {
  showNotificationDeleteDialog.value = false;
};

const editNotification = async (item: INotification) => {
  editingNotification.value = item;
  showNotificationDialog.value = true;
};

const newNotification = async () => {
  const notification = { ...defaultNotification };

  notifications.value.push(notification);

  editingNotification.value = notification;
  showNotificationDialog.value = true;
};

const openNotificationsDeleteDialog = async (item: INotification) => {
  editingNotification.value = item;
  showNotificationDeleteDialog.value = true;
};

const notificationDeleteItemOk = async () => {
  notifications.value = notifications.value.filter((s) => s !== editingNotification.value);

  closeDeleteDialog();
};
</script>

<template>
  <v-data-table :sort-by="sortNotificationsBy" :items-per-page="itemsPerPage" :headers="tableNotificationsHeaders" :items="notifications" density="compact" item-value="name">
    <template v-slot:top>
      <v-toolbar density="compact">
        <v-toolbar-title>{{ props.label }}</v-toolbar-title>
        <v-spacer />
        <v-btn color="secondary" variant="outlined" class="mr-5" v-if="allowNew" @click="newNotification()">
          {{ t('notificationEditor.new_notification') }}
        </v-btn>

        <v-dialog v-model="showNotificationDialog" max-width="500px">
          <v-card>
            <v-toolbar density="compact" color="dialog-header">
              <v-toolbar-title>{{ t('general.edit') }}</v-toolbar-title>
            </v-toolbar>

            <v-card-text>
              <v-container>
                <v-row>
                  <v-text-field maxlength="25" v-model="editingNotification.name" :label='t("notificationEditor.name")' />
                </v-row>
                <v-row>
                  <v-text-field type="number" v-model.number="editingNotification.timeFromStart" :label='t("notificationEditor.time_from_start")' />
                </v-row>
                <v-row>
                  <v-switch v-model="editingNotification.buzzer" :label='t("notificationEditor.buzzer")' color="red" />
                </v-row>
                <v-row>
                  <v-textarea v-model="editingNotification.message" :label='t("notificationEditor.message")' />
                </v-row>
              </v-container>
            </v-card-text>

            <v-card-actions>
              <v-spacer />
              <v-btn color="blue-darken-1" variant="text" @click="closeDialog">
                {{ t('general.close') }}
              </v-btn>
            </v-card-actions>
          </v-card>
        </v-dialog>
        <v-dialog v-model="showNotificationDeleteDialog" max-width="500px">
          <v-card>
            <v-card-title class="text-h5">{{ t('general.delete_message') }}</v-card-title>
            <v-card-actions>
              <v-spacer />
              <v-btn color="blue-darken-1" variant="text" @click="closeDeleteDialog">{{ t('general.cancel')
              }}</v-btn>
              <v-btn color="blue-darken-1" variant="text" @click="notificationDeleteItemOk">{{t('general.ok')
              }}</v-btn>
              <v-spacer />
            </v-card-actions>
          </v-card>
        </v-dialog>
      </v-toolbar>
    </template>
    <template v-slot:[`item.actions`]="{ item }">
      <v-icon size="small" class="me-2" @click="editNotification(item)" :icon="mdiPencil" />
      <v-icon size="small" @click="openNotificationsDeleteDialog(item)" :icon="mdiDelete" />
    </template>
    <template v-slot:[`item.buzzer`]="{ item }">
      <v-checkbox-btn class="align-right justify-center" v-model="item.buzzer" disabled />
    </template>

  </v-data-table>
</template>
