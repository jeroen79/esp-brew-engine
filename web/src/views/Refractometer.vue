<script lang="ts" setup>
import { computed, ref } from "vue";
import { useI18n } from "vue-i18n";
const { t } = useI18n({ useScope: "global" });

const ogTypes = ["Brix", "Gravity"];
const ogType = ref("Brix");

const abvCalcMethods = ["Papazian", "Daniels"];
const abvCalcMethod = ref("Daniels");

const ogInput = ref<number>(20);
const fgInput = ref<number>(12);

const wortCorrectionFactor = ref(1);

const convertPlatoToGravity = (plato: number): number => plato / (258.6 - (plato / 258.2) * 227.1) + 1;

const convertGravityToPlato = (gravity: number): number => {
  const plato = -1 * 616.868 + 1111.14 * gravity - 630.272 * gravity ** 2 + 135.997 * gravity ** 3;
  return plato;
};

const convertGravityToBrix = (gravity: number) => ((182.4601 * gravity - 775.6821) * gravity + 1262.7794) * gravity - 669.5622;

const calcFBFromBrix = (startBrix: number, endBrix: number) => {
  const fg = 1.001843 - 0.002318474 * startBrix - 0.000007775 * (startBrix * startBrix) - 0.000000034 * (startBrix * startBrix * startBrix) + 0.00574 * endBrix + 0.00003344 * (endBrix * endBrix) + 0.000000086 * (endBrix * endBrix * endBrix);
  return fg;
};

const calcABVPapazian = (startSG: number, endSG: number) => {
  const abv = (startSG - endSG) * 131.25;
  return abv;
};

const calcABVDaniels = (startSG: number, endSG: number) => {
  const abv = ((76.08 * (startSG - endSG)) / (1.775 - startSG)) * (endSG / 0.794);
  return abv;
};

const ogInputCorrected = computed(() => {
  if (ogInput.value > 1000) {
    return ogInput.value / 1000;
  }
  return ogInput.value;
});

const fgInputCorrected = computed(() => {
  if (fgInput.value > 1000) {
    return fgInput.value / 1000;
  }
  return fgInput.value;
});

const ogPlato = computed(() => {
  if (ogType.value === "Brix") {
    const plato = ogInputCorrected.value / wortCorrectionFactor.value;
    return plato;
  }
  const plato = convertGravityToPlato(ogInputCorrected.value);
  return plato;
});

const ogSg = computed(() => {
  if (ogType.value === "Brix") {
    const plato = ogInputCorrected.value / wortCorrectionFactor.value;
    const sg = convertPlatoToGravity(plato);
    return sg;
  }
  const sg = ogInputCorrected.value;
  return sg;
});

const ogBrix = computed(() => {
  if (ogType.value === "Brix") {
    return ogInputCorrected.value;
  }
  return convertGravityToBrix(ogInputCorrected.value);
});

const ogPlatoRouned = computed(() => Math.round(ogPlato.value * 10) / 10);
const ogSgRouned = computed(() => Math.round(ogSg.value * 1000) / 1000);

const fgPlato = computed(() => {
  const plato = fgInputCorrected.value / wortCorrectionFactor.value;
  return plato;
});

const fgSG = computed(() => {
  const fg = calcFBFromBrix(ogBrix.value, fgInput.value);
  return fg;
});

const fgPlatoRounded = computed(() => Math.round(fgPlato.value * 10) / 10);

const fgRounded = computed(() => Math.round(fgSG.value * 1000) / 1000);

const abv = computed(() => {
  if (abvCalcMethod.value === "Papazian") {
    const abvc = calcABVPapazian(ogSg.value, fgSG.value);
    return abvc;
  }
  const abvc = calcABVDaniels(ogSg.value, fgSG.value);
  return abvc;
});

const abvRounded = computed(() => Math.round(abv.value * 100) / 100);

const abvPrimed = computed(() => abv.value + 0.3);

const abvPrimedRounded = computed(() => Math.round(abvPrimed.value * 100) / 100);
</script>

<template>
  <v-container class="spacing-playground pa-6" fluid>
    <v-form fast-fail @submit.prevent>
      <v-row>
        <v-col cols="12" md="3">
          <v-text-field v-model="ogInput" :label='t("refractometer.original_gravity")' />
        </v-col>
        <v-col cols="12" md="3">
          <v-select v-model="ogType" :items="ogTypes" item-text="caption" item-value="key" :label='t("refractometer.select")' />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field v-model="ogPlatoRouned" readonly :label='t("refractometer.original_gravity_plato")' />
        </v-col>
        <v-col cols="12" md="3">
          <v-text-field v-model="ogSgRouned" :label='t("refractometer.original_gravity_sg")' />
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="3">
          <v-text-field v-model="wortCorrectionFactor" :label='t("refractometer.wortCorrectionFactor")' />
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="3">
          <v-text-field v-model="fgInput" :label='t("refractometer.final_gravity_brix")' />
        </v-col>

        <v-col cols="12" md="3" />

        <v-col cols="12" md="3">
          <v-text-field v-model="fgPlatoRounded" readonly :label='t("refractometer.final_gravity_plato")' />
        </v-col>

        <v-col cols="12" md="3">
          <v-text-field v-model="fgRounded" readonly :label='t("refractometer.final_gravity_sg")' />
        </v-col>
      </v-row>

      <v-row>
        <v-col cols="12" md="3">
          <v-text-field v-model="abvRounded" readonly :label='t("refractometer.abv")' />
        </v-col>

        <v-col cols="12" md="3">
          <v-select v-model="abvCalcMethod" :items="abvCalcMethods" item-text="caption" item-value="key" :label='t("refractometer.method")' />
        </v-col>

      </v-row>

      <v-row>
        <v-col cols="12" md="3">
          <v-text-field v-model="abvPrimedRounded" readonly :label='t("refractometer.abv_sugar")' />
        </v-col>
      </v-row>

    </v-form>
  </v-container>
</template>
