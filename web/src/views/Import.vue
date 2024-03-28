<script lang="ts" setup>
import { mdiHelp } from '@mdi/js';
import WebConn from '@/helpers/webConn';
import ImportedBeer from '@/classes/ImportedBeer';
import { inject, ref } from 'vue';
import { IImportedBeer } from '@/interfaces/IImportedBeer';
import { IMashStep } from '@/interfaces/IMashStep';
import { useAppStore } from '@/store/app';
import { INotification } from '@/interfaces/INotification';
import StepEditor from '@/components/StepEditor.vue';
import NotificationEditor from '@/components/NotificationEditor.vue';
import TemperatureScale from '@/enums/TemperatureScale';
import { IMashSchedule } from '@/interfaces/IMashSchedule';
import { groupBy } from '@/helpers/grouping';
import { ITitleValue } from '@/interfaces/ITitleValue';

const webConn = inject<WebConn>('webConn');
const appStore = useAppStore();

const alert = ref<string>('');
const alertType = ref<'error' | 'success' | 'warning' | 'info'>('info');

const chosenFiles = ref<Array<File>>([]);
const fileData = ref<any>();

const xmlDoc = ref<Document>();
const foundRecipies = ref<Array<ITitleValue>>();
const showSelection = ref(false);

const temporary = ref(true);

const importedBeer = ref<IImportedBeer>({
  name: '',
  mashSteps: [],
  mashNotifications: [],
  mashNotificationsGrouped: [],
  boilSteps: [],
  boilNotifications: [],
  boilNotificationsGrouped: [],
});

const parseBeer = (beerToImport: Element) => {
  const beer = new ImportedBeer();
  beer.name = beerToImport.getElementsByTagName('NAME')[0].textContent;

  // Convert Mash Steps
  const mashSteps = beerToImport.getElementsByTagName('MASH')[0].getElementsByTagName('MASH_STEPS')[0].getElementsByTagName('MASH_STEP');

  const boilTime = parseInt(beerToImport.getElementsByTagName('BOIL_TIME')[0].textContent || '0', 10);
  let totalMashTime = 0;
  let grainAddTime = 0;

  if (mashSteps != null && mashSteps.length > 0) {
    for (let i = 0; i < mashSteps.length; i += 1) {
      const step = mashSteps[i];

      const stepType = step.getElementsByTagName('TYPE')[0].textContent;
      if (stepType !== 'Temperature' && stepType !== 'Infusion') {
        continue;
      }

      let stepName = step.getElementsByTagName('NAME')[0].textContent;

      if (stepName == null) {
        stepName = `Step ${i}`;
      }

      const stepTime = parseInt(step.getElementsByTagName('STEP_TIME')[0].textContent || '0', 10);

      let stepStepTime = 0;
      if (step.getElementsByTagName('RAMP_TIME').length > 0) { //it seems some xml's like brewfather don't always include this
        stepStepTime = parseInt(step.getElementsByTagName('RAMP_TIME')[0].textContent || '0', 10);
      }
       
      let stepTemperature = parseInt(step.getElementsByTagName('STEP_TEMP')[0].textContent || '0', 10);// always in degree C

      // convert to F when device is in F mode
      if (appStore.temperatureScale === TemperatureScale.Fahrenheit) {
        // Fahrenheit = (Celsius * 1.8) + 32
        const tempF = (stepTemperature * 1.8) + 32;
        stepTemperature = Math.round(tempF);
      }

      const mashStep: IMashStep = {
        index: i,
        name: stepName,
        temperature: stepTemperature,
        stepTime: stepStepTime,
        time: stepTime,
        extendStepTimeIfNeeded: true,
      };
      beer.mashSteps.push(mashStep);

      // add start sparge notifiation, sprage type doesn't exist yet in beerxml
      if (stepName?.toLowerCase().indexOf('sparge') > -1) {
        const notification: INotification = {
          name: 'Sparge',
          message: 'Start Sparge',
          timeFromStart: totalMashTime, //time is start of this step
          timePoint: 0,
          buzzer: true,
          done: false
        };

        beer.mashNotifications.push(notification);
      }


      totalMashTime += stepStepTime + stepTime;

      // grain add time is first step time
      if (i === 0) {
        grainAddTime = stepStepTime;
      }
    }
  }

  // Add grain add Notification
  const fermentables = beerToImport.getElementsByTagName('FERMENTABLES')[0].getElementsByTagName('FERMENTABLE');
  if (fermentables != null && fermentables.length > 0) {
    let fermentablesText = '';
    let lateBoilText = '';

    for (let i = 0; i < fermentables.length; i += 1) {
      const fermentable = fermentables[i];
      const fermentableName = fermentable.getElementsByTagName('NAME')[0].textContent;
      const fermentableAmount = parseFloat(fermentable.getElementsByTagName('AMOUNT')[0].textContent || '0') * 1000; // all beerxml weight are in kg
      
      let isMashedString:string | null = null;
      if (fermentable.getElementsByTagName('RECOMMEND_MASH').length > 0) { 
        isMashedString = fermentable.getElementsByTagName('RECOMMEND_MASH')[0].textContent;
      }else if(fermentable.getElementsByTagName('NOT_FERMENTABLE').length > 0){ //brewfather seems to use non standard NOT_FERMENTABLE tag
        isMashedString = fermentable.getElementsByTagName('NOT_FERMENTABLE')[0].textContent; 
      }

      const isMashed = (isMashedString != null && isMashedString.toLowerCase() === 'true');

      const afterBoilString = fermentable.getElementsByTagName('ADD_AFTER_BOIL')[0].textContent;
      const afterBoil = (afterBoilString != null && afterBoilString.toLowerCase() === 'true');

      if (isMashed) {
        fermentablesText += `${fermentableAmount}g of ${fermentableName}\n`;
      } else if (afterBoil) {
        lateBoilText += `${fermentableAmount}g of ${fermentableName}\n`;
      }
    }

    if (fermentablesText !== '') {
      const notification: INotification = {
        name: 'Fermentables',
        message: fermentablesText,
        timeFromStart: grainAddTime,
        timePoint: 0,
        buzzer: true,
      };

      beer.mashNotifications.push(notification);
    }

    if (lateBoilText !== '') {
      const notification: INotification = {
        name: 'Late Additions',
        message: lateBoilText,
        timeFromStart: boilTime,
        timePoint: 0,
        buzzer: true,
      };

      beer.boilNotifications.push(notification);
    }
  }

  // Convert Boil Steps
  let boilTemp = 100;

  // Some beerxmls seems to include eqyuipent that can contain biol settings in C
  if (beerToImport.getElementsByTagName('EQUIPMENT').length > 0) {
    const equipment = beerToImport.getElementsByTagName('EQUIPMENT')[0];
    if (equipment.getElementsByTagName('BOILING_POINT').length > 0) {
      boilTemp = parseInt(equipment.getElementsByTagName('BOILING_POINT')[0].textContent || '0', 10);
    }
  }

  // convert to F when device is in F mode
  if (appStore.temperatureScale === TemperatureScale.Fahrenheit) {
    // Fahrenheit = (Celsius * 1.8) + 32
    const tempF = (boilTemp * 1.8) + 32;
    boilTemp = Math.round(tempF);
  }

  const boilStep: IMashStep = {
    index: 0,
    name: 'Boil',
    temperature: boilTemp,
    stepTime: 0,
    time: boilTime,
    extendStepTimeIfNeeded: true,
  };
  beer.boilSteps.push(boilStep);

  // Convert Hops to Notifications
  const hops = beerToImport.getElementsByTagName('HOPS')[0].getElementsByTagName('HOP');
  if (hops != null && hops.length > 0) {
    for (let i = 0; i < hops.length; i += 1) {
      const hop = hops[i];
      const hopUse = hop.getElementsByTagName('USE')[0].textContent || '';

      // we don't run that long, dry hop is not our job
      if (hopUse === 'Dry Hop' || hopUse === 'Dry') {
        continue;
      }

      const hopName = hop.getElementsByTagName('NAME')[0].textContent;
      const hopAmount = parseFloat(hop.getElementsByTagName('AMOUNT')[0].textContent || '0') * 1000; // all beerxml weight are in kg
      const hopInfusTime = parseInt(hop.getElementsByTagName('TIME')[0].textContent || '0', 10);// this is not the add time but the infusion time

      if (hopUse === 'Boil') {
        const hopAddTime = boilTime - hopInfusTime;

        const notification: INotification = {
          name: 'Hop',
          message: `${hopAmount}g of ${hopName}`,
          timeFromStart: hopAddTime,
          timePoint: 0,
          buzzer: true,
        };

        beer.boilNotifications.push(notification);
      } else if (hopUse === 'Mash') {
        const hopAddTime = totalMashTime - hopInfusTime;

        const notification: INotification = {
          name: 'Hop',
          message: `${hopAmount}g of ${hopName}`,
          timeFromStart: hopAddTime,
          timePoint: 0,
          buzzer: true,
        };

        beer.mashNotifications.push(notification);
      }
    }
  }

  // Convert Misc like herbs etc
  const miscs = beerToImport.getElementsByTagName('MISCS')[0].getElementsByTagName('MISC');
  if (miscs != null && miscs.length > 0) {
    for (let i = 0; i < miscs.length; i += 1) {
      const misc = miscs[i];
      const miscUse = misc.getElementsByTagName('USE')[0].textContent || '';

      if (miscUse === 'Primary') {
        continue;
      }

      const miscName = misc.getElementsByTagName('NAME')[0].textContent;
      const miscType = misc.getElementsByTagName('TYPE')[0].textContent;
      const miscInfusTime = parseInt(misc.getElementsByTagName('TIME')[0].textContent || '0', 10);// this is not the add time but the infusion time
      const miscAmount = parseFloat(misc.getElementsByTagName('AMOUNT')[0].textContent || '0') * 1000; // all beerxml weight are in kg

      if (miscUse === 'Boil') {
        const miscAddTime = boilTime - miscInfusTime;

        const notification: INotification = {
          name: `${miscType}`,
          message: `${miscAmount}g of ${miscName}`,
          timeFromStart: miscAddTime,
          timePoint: 0,
          buzzer: true,
        };

        beer.boilNotifications.push(notification);
      } else if (miscUse === 'Mash') {
        const notification: INotification = {
          name: `${miscType}`,
          message: `${miscAmount}g of ${miscName}`,
          timeFromStart: 0,
          timePoint: 0,
          buzzer: true,
        };

        beer.mashNotifications.push(notification);
      }
    }
  }

  // we need to group our notifications on time and merge
  const groupedMash = groupBy(beer.mashNotifications, (n) => n.timeFromStart);
  beer.mashNotificationsGrouped = [];
  groupedMash.forEach((group, key) => {
    if (group.values.length === 1) { // only one so just add
      beer.mashNotificationsGrouped.push(group[0]);
    } else {
      // else combine
      let newName = '';
      let newMessage = '';
      let buzzer = false;

      group.forEach((notification) => {
        // when not yet in name add
        if (newName.indexOf(notification.name) === -1) {
          if (newName === '') {
            newName = notification.name;
          } else {
            newName += ` / ${notification.name}`;
          }
        }
        newMessage += `${notification.message}\n`;

        if (notification.buzzer) {
          buzzer = true;
        }
      });

      const newNotification: INotification = {
        name: newName,
        message: newMessage,
        timeFromStart: key,
        timePoint: 0,
        buzzer,
      };
      beer.mashNotificationsGrouped.push(newNotification);
    }
  });

  const groupedBoil = groupBy(beer.boilNotifications, (n) => n.timeFromStart);
  beer.boilNotificationsGrouped = [];
  groupedBoil.forEach((group, key) => {
    if (group.values.length === 1) { // only one so just add
      beer.boilNotificationsGrouped.push(group[0]);
    } else {
      // else combine
      let newName = '';
      let newMessage = '';
      let buzzer = false;

      group.forEach((notification) => {
        // when not yet in name add
        if (newName.indexOf(notification.name) === -1) {
          if (newName === '') {
            newName = notification.name;
          } else {
            newName += ` / ${notification.name}`;
          }
        }
        newMessage += `${notification.message}\n`;

        if (notification.buzzer) {
          buzzer = true;
        }
      });

      const newNotification: INotification = {
        name: newName,
        message: newMessage,
        timeFromStart: key,
        timePoint: 0,
        buzzer,
      };
      beer.boilNotificationsGrouped.push(newNotification);
    }
  });

  importedBeer.value = beer;
};

const parseFile = () => {
  const parser = new DOMParser();
  xmlDoc.value = parser.parseFromString(fileData.value, 'text/xml');

  const recipeCount = xmlDoc.value.getElementsByTagName('RECIPE').length;
  if (recipeCount === 0) {
    alert.value = 'No Recipes found in Beer XML!';
    alertType.value = 'warning';
  } else if (recipeCount === 1) {
    const firstBeer = xmlDoc.value.getElementsByTagName('RECIPE')[0];

    parseBeer(firstBeer);
  } else {
    foundRecipies.value = [];
    const recipes = xmlDoc.value.getElementsByTagName('RECIPE');

    for (let i = 0; i < recipes.length; i += 1) {
      const name = recipes[i].getElementsByTagName('NAME')[0].textContent;
      foundRecipies.value.push({
        title: name || "unnamed",
        value: i
      });
    }

    //Show dialog to select a beer
    showSelection.value = true;

  }
};

const selectBeer = (args: any) => {
  if (xmlDoc.value == null) {
    return;
  }

  showSelection.value = false;

  if (args.id > -1) {
    const beer = xmlDoc.value.getElementsByTagName('RECIPE')[args.id];
    parseBeer(beer);
  }
};

const fileSelected = (event: any) => {
  if (chosenFiles.value == null || chosenFiles.value.length === 0) {
    alert.value = 'No file choosen';
    alertType.value = 'warning';
  }

  const file: Blob = chosenFiles.value[0];
  const reader = new FileReader();
  reader.readAsText(file);
  reader.onload = () => {
    fileData.value = reader.result;
    parseFile();
  };
};

const refreshAppStoreSchedules = async () => {
  await appStore.getMashSchedules();
};

const upload = async () => {
  if (importedBeer.value == null || importedBeer.value.name == null || importedBeer.value.name === '') {
    alert.value = 'Please import beer first!';
    alertType.value = 'warning';
    return;
  }

  const mashName = `${importedBeer.value.name} (Mash)`;

  const newMashSchedule: IMashSchedule = {
    name: mashName,
    boil: false,
    temporary: temporary.value,
    steps: [...importedBeer.value.mashSteps],
    notifications: [...importedBeer.value.mashNotificationsGrouped],
  };

  const requestData = {
    command: (temporary.value) ? 'SetMashSchedule' : 'SaveMashSchedule',
    data: newMashSchedule,
  };

  const result = await webConn?.doPostRequest(requestData);

  if (result?.message != null) {
    alertType.value = 'error';
    alert.value = result?.message;
  }

  const boilName = `${importedBeer.value.name} (Boil)`;

  const newBoilSchedule: IMashSchedule = {
    name: boilName,
    boil: true,
    temporary: temporary.value,
    steps: [...importedBeer.value.boilSteps],
    notifications: [...importedBeer.value.boilNotificationsGrouped],
  };

  const requestData2 = {
    command: (temporary.value) ? 'SetMashSchedule' : 'SaveMashSchedule',
    data: newBoilSchedule,
  };

  const result2 = await webConn?.doPostRequest(requestData2);

  if (result2?.message != null) {
    alertType.value = 'error';
    alert.value = result2?.message;
  } else {
    alertType.value = 'info';
    alert.value = 'Schedules Imported!';

    refreshAppStoreSchedules();
  }
};

const temporaryChanged = () => {
  // we need to check if we have reached our max
  if (!temporary.value) {
    const mashName = `${importedBeer.value.name} (Mash)`;
    const boilName = `${importedBeer.value.name} (Boil)`;

    const count = appStore.mashSchedules.filter((s) => s.name !== mashName && s.name !== boilName).length;
    if (count > appStore.maxSchedules.valueOf() - 2) {
      alert.value = `Only ${appStore.maxSchedules} Schedules can be saved!`;
      alertType.value = 'warning';
      temporary.value = true;
    }
  }
};

</script>

<template>
  <v-container class="spacing-playground pa-6" fluid>
    <v-alert :type="alertType" v-if="alert" closable @click:close="alert = ''">{{ alert }}</v-alert>

    <v-dialog v-model="showSelection" max-width="500px">
      <v-card>
        <v-toolbar density="compact" color="dialog-header">
          <v-toolbar-title>Select Beer</v-toolbar-title>
        </v-toolbar>

        <v-card-text>
          <v-container>
            <v-list :items="foundRecipies" @click:select="selectBeer"></v-list>
          </v-container>
        </v-card-text>

        <v-card-actions>
          <v-spacer />
          <v-btn color="blue-darken-1" variant="text" @click="showSelection = false">
            Cancel
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <v-form fast-fail @submit.prevent>
      <v-row>
        <v-file-input label="Import BeerXML" accept="text/xml" :multiple="false" v-model="chosenFiles" @change="fileSelected" />
      </v-row>
      <v-row>
        <v-col cols="12" sm="6" md="6" lg="3">
          <v-text-field v-model="importedBeer.name" readonly label="Name" />
        </v-col>
        <v-col cols="12" md="3">
          <v-switch v-model="temporary" label="Temporary" color="green" @change="temporaryChanged">
            <template v-slot:append>
              <v-tooltip text="By default imports are not saved to flash and only kept in memory, only disable this if you want to save the schedules permanently.">
                <template v-slot:activator="{ props }">
                  <v-icon size="small" v-bind="props">{{ mdiHelp }}</v-icon>
                </template>
              </v-tooltip>
            </template>
          </v-switch>
        </v-col>
        <v-col cols="12" md="3">
          <v-btn color="success" class="mt-4 mr-2" @click="upload"> Import </v-btn>
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12">
          <StepEditor v-model="importedBeer.mashSteps" :items-per-page="10" :allow-new="true" label="Mash Steps" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12">
          <NotificationEditor v-model="importedBeer.mashNotificationsGrouped" :items-per-page="10" :allow-new="true" label="Mash Notifications" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12">
          <StepEditor v-model="importedBeer.boilSteps" :items-per-page="10" :allow-new="true" label="Boil Steps" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12">
          <NotificationEditor v-model="importedBeer.boilNotificationsGrouped" :items-per-page="10" :allow-new="true" label="Boil Notifications" />
        </v-col>
      </v-row>
    </v-form>
  </v-container>

</template>
