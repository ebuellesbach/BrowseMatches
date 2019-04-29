<template>
  <div id="overall">

    <div id="header"> 
       <router-link :to="{ name: 'DisplayWrapper', params: {form: form}}">
        Search
      </router-link>
    </div>

    <br>

    <div id="form">
      <b-form>
        <b-form-group
          id="input-group-1"
          label="Target Fragment ID:"
          label-for="input-1"
        >
          <b-form-input
            id="input-1"
            v-model="form.tgt"
            type="text"
          ></b-form-input>
          <b-form-checkbox v-modal="form.tgtLike" :value="true">LIKE?</b-form-checkbox>
        </b-form-group>

        <b-form-group
          id="input-group-2"
          label="Source Fragment ID:"
          label-for="input-2"
        >
          <b-form-input
            id="input-2"
            v-model="form.src"
            type="text"
          ></b-form-input>
          <b-form-checkbox v-modal="form.srcLike" :value="true">LIKE?</b-form-checkbox>
        </b-form-group>

        <b-form-group
          id="input-group-3"
          label="Status:"
          label-for="input-3"
        >
          <b-form-select
            id="input-3"
            v-model="form.evalu"
            :options="[
              { text: '', value: null },
              { text: 'Confirmed', value: 1 },
              { text: 'Rejected', value: 0 },
              { text: 'Unknown', value: -1 }]"
          ></b-form-select>
        </b-form-group>

        <b-form-group
          id="input-group-4"
          label="Sort By:"
          label-for="input-4"
        >
          <b-form-select
            id="input-4"
            v-model="form.sortType"
            :options="[
              { text: '', value: null },
              { text: 'Error', value: 'error' },
              { text: 'Probability', value: 'probability' }]"
          ></b-form-select>
        </b-form-group>

         <b-form-group
          id="input-group-5"
          label="Sort Order:"
          label-for="input-5"
        >
          <b-form-select
            id="input-5"
            v-model="form.sortOrder"
            :options="[
              { text: '', value: null },
              { text: 'Ascending', value: 'asc' },
              { text: 'Decending', value: 'desc' }]"
          ></b-form-select>
        </b-form-group>

      </b-form>
    </div>
  	
  </div>
</template>

<script>
export default {
  name: 'Overview',
  data () {
  	return {
      form: {
        tgt: '',
        tgtLike: false,
        src: '',
        srcLike: false,
        evalu: null,
        txt: '',
        sortType: null,
        sortOrder: null,
      }
	   }
  }, 
  methods: {
    clearSearch(){
      this.tgt = '';
      this.src = '';
      this.evalu = null;
      this.txt = '';
      this.sortType = null;
      this.sortOrder = null;
    }
  }
}
</script>

<style scoped>

#overall {
  width: 400px;
  margin: 0 auto;
  padding: 20px;
}

#form, #header {
  background-color: #ebfafa;
  padding: 5px;
  border-radius: 5px;
}
</style>