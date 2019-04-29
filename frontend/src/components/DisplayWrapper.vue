<template>
	<b-container>
		<Display :params="params" 
			:dataTotal="dataTotal"
			@tgt-click="onTgtClick"
      @src-click="onSrcClick" />
	</b-container>
</template>

<script>
import Display from './Display.vue';

export default {
  name: 'DisplayWrapper',
  props: ['form'],
  components: {
    Display
  },
  data () {
  	return {
	  	params: {
	  		tgt: this.form.tgt,
	        tgtLike: this.form.tgtLike,
	        src: this.form.src,
	        srcLike: this.form.srcLike,
	        evalu: this.form.evalu,
	        txt: this.form.txt,
	        sortType: this.form.sortType,
	        sortOrder: this.form.sortOrder,
	  	}
  	}
  }, 
  asyncComputed: {
    async dataTotal() {
      let url = new URL("http://localhost:3000/data/count");
      Object.keys(this.params).forEach(key =>
        url.searchParams.append(key,this.params[key]));
      let res = await fetch(url);
      let json = await res.json();

      if (json.err != null){
        console.log(json.err);
        return 0;
      } else {
        return json.count;
      }
    }
  },
  methods: {
  	onTgtClick: function (tgt){
  		let newParams = {
  			  tgt: tgt,
	        tgtLike: false,
	        src: '',
	        srcLike: false,
	        evalu: null,
	        txt: '',
	        sortType: null,
	        sortOrder: null
  		}
  		this.params = newParams;
  	},
    onSrcClick: function (src){
      let newParams = {
          tgt: '',
          tgtLike: false,
          src: src,
          srcLike: false,
          evalu: null,
          txt: '',
          sortType: null,
          sortOrder: null
      }
      this.params = newParams;
    }
  }
}

</script>