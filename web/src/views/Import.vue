<script lang="ts" setup>
import { mdiDelete, mdiPencil } from '@mdi/js';
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

const webConn = inject<WebConn>('webConn');
const appStore = useAppStore();

const alert = ref<string>('');
const alertType = ref<'error' | 'success' | 'warning' | 'info' >('info');

const chosenFiles = ref<Array<File>>([]);
const fileData = ref<any>();

const importedBeer = ref<IImportedBeer>({
  name: '',
  mashSteps: [],
  mashNotifications: [],
  boilSteps: [],
  boilNotifications: [],
});

// Mash steps

// Boil Step

// Mash notifications
const parseFile = () => {
  const parser = new DOMParser();
  const xmlDoc = parser.parseFromString(fileData.value, 'text/xml');
  const firstBeer = xmlDoc.getElementsByTagName('RECIPE')[0];

  if (firstBeer == null) {
    alert.value = 'Not Recipe found in Beer XML!';
    alertType.value = 'warning';
    return;
  }

  const beer = new ImportedBeer();
  beer.name = firstBeer.getElementsByTagName('NAME')[0].textContent;

  // Convert Mash Steps
  const mashSteps = firstBeer.getElementsByTagName('MASH')[0].getElementsByTagName('MASH_STEPS')[0].getElementsByTagName('MASH_STEP');

  const boilTime = parseInt(firstBeer.getElementsByTagName('BOIL_TIME')[0].textContent || '0', 10);
  let totalMashTime = 0;
  let grainAddTime = 0;

  if (mashSteps != null && mashSteps.length > 0) {
    for (let i = 0; i < mashSteps.length; i += 1) {
      const step = mashSteps[i];

      const stepType = step.getElementsByTagName('TYPE')[0].textContent;
      if (stepType !== 'Temperature' && stepType !== 'Infusion') {
        continue;
      }

      const stepName = step.getElementsByTagName('NAME')[0].textContent;
      const stepTime = parseInt(step.getElementsByTagName('STEP_TIME')[0].textContent || '0', 10);
      const stepStepTime = parseInt(step.getElementsByTagName('RAMP_TIME')[0].textContent || '0', 10);
      let stepTemperature = parseInt(step.getElementsByTagName('INFUSE_TEMP')[0].textContent || '0', 10);// always in degree C

      // convert to F when device is in F mode
      if (appStore.temperatureScale === TemperatureScale.Fahrenheit) {
        // Fahrenheit = (Celsius * 1.8) + 32
        const tempF = (stepTemperature * 1.8) + 32;
        stepTemperature = Math.round(tempF);
      }

      const mashStep:IMashStep = {
        index: i,
        name: stepName || `Step ${i}`,
        temperature: stepTemperature,
        stepTime: stepStepTime,
        time: stepTime,
        extendStepTimeIfNeeded: true,
      };
      beer.mashSteps.push(mashStep);

      totalMashTime += stepStepTime + stepTime;

      // grain add time is first step time
      if (i === 0) {
        grainAddTime = stepStepTime;
      }
    }
  }

  // Add grain add Notification
  const fermentables = firstBeer.getElementsByTagName('FERMENTABLES')[0].getElementsByTagName('FERMENTABLE');
  if (fermentables != null && fermentables.length > 0) {
    let fermentablesText = '';
    let lateBoilText = '';

    for (let i = 0; i < fermentables.length; i += 1) {
      const fermentable = fermentables[i];
      const fermentableName = fermentable.getElementsByTagName('NAME')[0].textContent;
      const fermentableAmount = parseFloat(fermentable.getElementsByTagName('AMOUNT')[0].textContent || '0') * 1000; // all beerxml weight are in kg
      const isMashedString = fermentable.getElementsByTagName('IS_MASHED')[0].textContent;
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
      const notification:INotification = {
        name: 'Add Fermentables',
        message: fermentablesText,
        timeFromStart: grainAddTime,
        timePoint: 0,
        buzzer: true,
      };

      beer.mashNotifications.push(notification);
    }

    if (lateBoilText !== '') {
      const notification:INotification = {
        name: 'Add Late Additions',
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
  const equipment = firstBeer.getElementsByTagName('EQUIPMENT')[0];
  if (equipment != null) {
    boilTemp = parseInt(equipment.getElementsByTagName('BOILING_POINT')[0].textContent || '0', 10);
  }

  // convert to F when device is in F mode
  if (appStore.temperatureScale === TemperatureScale.Fahrenheit) {
    // Fahrenheit = (Celsius * 1.8) + 32
    const tempF = (boilTemp * 1.8) + 32;
    boilTemp = Math.round(tempF);
  }

  const boilStep:IMashStep = {
    index: 0,
    name: 'Boil',
    temperature: boilTemp,
    stepTime: 0,
    time: boilTime,
    extendStepTimeIfNeeded: true,
  };
  beer.boilSteps.push(boilStep);

  // Convert Hops to Notifications
  const hops = firstBeer.getElementsByTagName('HOPS')[0].getElementsByTagName('HOP');
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

        const notification:INotification = {
          name: 'Add Hop',
          message: `${hopAmount}g of ${hopName}`,
          timeFromStart: hopAddTime,
          timePoint: 0,
          buzzer: true,
        };

        beer.boilNotifications.push(notification);
      } else if (hopUse === 'Mash') {
        const hopAddTime = totalMashTime - hopInfusTime;

        const notification:INotification = {
          name: 'Add Hop',
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
  const miscs = firstBeer.getElementsByTagName('MISCS')[0].getElementsByTagName('MISC');
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

        const notification:INotification = {
          name: `Add ${miscType}`,
          message: `${miscAmount}g of ${miscName}`,
          timeFromStart: miscAddTime,
          timePoint: 0,
          buzzer: true,
        };

        beer.boilNotifications.push(notification);
      } else if (miscUse === 'Mash') {
        const notification:INotification = {
          name: `Add ${miscType}`,
          message: `${miscAmount}g of ${miscName}`,
          timeFromStart: 0,
          timePoint: 0,
          buzzer: true,
        };

        beer.mashNotifications.push(notification);
      }
    }
  }

  importedBeer.value = beer;
};

const fileSelected = (event:any) => {
  if (chosenFiles.value == null || chosenFiles.value.length === 0) {
    alert.value = 'No file choosen';
    alertType.value = 'warning';
  }

  const file:Blob = chosenFiles.value[0];
  console.log(chosenFiles.value);

  const reader = new FileReader();
  reader.readAsText(file);
  reader.onload = () => {
    fileData.value = reader.result;
    parseFile();
  };
};

const debug = () => {
  console.log('step:', importedBeer.value.mashSteps);
};

</script>

<template>
  <v-btn color="success" class="mt-4 mr-2" @click="debug"> Debug </v-btn>
  <v-container class="spacing-playground pa-6" fluid>
    <v-alert :type="alertType" v-if="alert" closable @click:close="alert = ''">{{alert}}</v-alert>
    <v-form fast-fail @submit.prevent>
      <v-row>
        <v-file-input label="Import BeerXML" accept="text/xml" :multiple="false" v-model="chosenFiles" @change="fileSelected" />
      </v-row>
      <v-row>
        <v-col cols="12" md="6">
          <v-text-field v-model="importedBeer.name" readonly label="Name" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12">
          <StepEditor v-model="importedBeer.mashSteps" :items-per-page="10" :allow-new="true" label="Mash Steps" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12">
          <NotificationEditor v-model="importedBeer.mashNotifications" :items-per-page="10" :allow-new="true" label="Mash Notifications" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12">
          <StepEditor v-model="importedBeer.boilSteps" :items-per-page="10" :allow-new="true" label="Boil Steps" />
        </v-col>
      </v-row>
      <v-row>
        <v-col cols="12">
          <NotificationEditor v-model="importedBeer.boilNotifications" :items-per-page="10" :allow-new="true" label="Boil Notifications" />
        </v-col>
      </v-row>
    </v-form>
  </v-container>

</template>
