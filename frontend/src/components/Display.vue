<template>
  <b-container fluid >

    <br>
    <!-- Header section -->
    <b-row align-h="center">
      <b-col sm="0" md="4">
      </b-col>
      <b-col>
        <!-- Pagination -->
        <b-pagination align="center" size="md" :total-rows="getRowCount()" v-model="displayPage" :per-page="perPage" />
      </b-col>
      <b-col>
        <!-- Buttons for tile size -->
        <b-button id="i-bttn" v-b-tooltip.hover title="Increase Tile Size" v-on:click="changeTileSize('increase')" :disabled="tile.height >= 500"> Larger </b-button>
        <b-button id="d-bttn" v-b-tooltip.hover title="Decrease Tile Size" v-on:click="changeTileSize('decrease')" :disabled="tile.height <= 100"> Smaller </b-button>
      </b-col>
    </b-row>
    
    <!-- Page display -->
    <b-row>
      <Page :data="displayData" 
            :tile="tile" 
            :rows="rows" 
            :cols="cols" 
            @key-page-nav="onPageNav"
            @change-vote="onChangeVote"
            @change-comment="onChangeComment"
            @tgt-click="onTgtClick"
            @src-click="onSrcClick"/>
  	</b-row>
  
  </b-container>
</template>

<script>
import Page from './Page.vue';

export default {
  name: 'Display',
  props: ['params', 'dataTotal'],
  components: {
    Page
  },
  data () {
  	return {
      
  		displayPage: 1,   // Current page displayed
      // displayData: [],  // List of data to be displayed
      tile: {           // Data to size tiles
        height: 140,
        width: 200,
        margin: 5,
      },
      blockSize: 120,   // How many matches to get at once
      blockPage: 1,     // What page of blocks we are on
      dataList: [],     // List of data
      dataCachePrior: { // Cache of prior block page
        page: 0,
        data: []
      },
      dataCacheNext: { // Cache of next block page
        page: 0,
        data: []
      }
  	}
  },
  computed: {
    cols: function (){
      let w = this.tile.width + (this.tile.margin * 4);
      let c = Math.floor(this.$root.$data.window.width / w)
      if (c == 0){
        c = 1;
      }
      return c;
    },
    rows: function (){
      let h = this.tile.height + this.tile.margin;
      let r = Math.floor(this.$root.$data.window.height / h)

      // Make space for the header
      if (this.tile.height == 200) {
        r -= 1;
      }
      if (this.tile.height < 200) {
        r -= 1;
      } 
      if (this.tile.height < 150) {
        r -= 1;
      } 
      if (this.tile.height < 100) {
        r -= 1;
      }
      
      // Scroll downwards only on the smallest screens
      if (this.cols < 2) {
        r = 5;
      } else if (r == 0){
        r = 1;
      }

      return r;
    },
    perPage: function (){
      return this.cols * this.rows;
    },
    displayData: function (){
      // console.log("setDisplayData");
      let pagePerBlock = Math.ceil(this.blockSize / this.perPage);
      
      let pageInBlock = this.displayPage % pagePerBlock;
      if (pageInBlock == 0){
        pageInBlock = pagePerBlock;
      }

      let start = ((pageInBlock - 1) * this.perPage);
      let end = ((pageInBlock - 1) * this.perPage) + this.perPage;

      if (this.dataList[end]){
        return this.dataList.slice(start, end);
      } else {
        return this.dataList.slice(start);
      }
      
    },
    pageCount: function () {
      return Math.ceil(this.getRowCount() / this.perPage);
    }
  },
  asyncComputed: {
    // Object: with total, page, perpage, and block
    async dataBlock() {

      console.log("asnyc computed dataBlock");

      // If cache for the current page, return that, update caches
      if (this.dataCacheNext.page == this.blockPage){
          let data = this.dataCacheNext.data;
          if (this.dataList != []){
            this.dataCachePrior.page == this.blockPage - 1;
            this.dataCachePrior.data = this.dataList
          } else {
            this.setDataCachePrior();
          }
          this.setDataCacheNext();
          return data;

      } else if (this.dataCachePrior.page == this.blockPage){
          let data = this.dataCachePrior.data;
          if (this.dataList != []){
            this.dataCacheNext.page == this.blockPage + 1;
            this.dataCacheNext.data = this.dataList
          } else {
            this.setDataCacheNext();
          }
          this.setDataCachePrior();
          return data;
      }
        
      // Else set both caches, and then return the data
      // NOTE: The ssh connection can only process one request at a time so you can only ever load one at a time, can only build cache forwards
      this.setDataCachePrior();
      this.setDataCacheNext();
      
      let params = { 
        page: this.blockPage,
        perPage: this.blockSize 
      };
      let data = await this.getData(params);

      // Need to set these here so they were able to be used in other properites then updated
      this.dataList = data.list;
      return data;
    }

  },
  methods: {

    // Given params: return the data associated with them
    getData: async function(params){

      let url = new URL("http://localhost:3000/data");

      // Need to add user ID to params
      params["uid"] = localStorage.getItem('user');

      // Append passed in params
      Object.keys(params).forEach(key =>
        url.searchParams.append(key,params[key]));
      // Apend overall query params
      Object.keys(this.params).forEach(key =>
        url.searchParams.append(key,this.params[key]));
      let res = await fetch(url);
      let json = await res.json();
      return json;
    },
    setDataCacheNext: async function(){
      // If cache is not already set
      let next = this.blockPage + 1;
      if (next != this.dataCacheNext.page){
        let params = {
          page: next,
          perPage: this.blockSize
        };
        let data = await this.getData(params);
        this.dataCacheNext.data = data;
        this.dataCacheNext.page = next;
      }
    },
    setDataCachePrior: async function (){
      // If cache is not already set
      let prior = this.blockPage - 1;
      if (prior != this.dataCachePrior.page){
        let params = {
          page: prior,
          perPage: this.blockSize
        };
        let data = await this.getData(params);
        this.dataCachePrior.data = data;
        this.dataCachePrior.page = prior;
      }
    },
    // Calls API to update match with ID to params
    updateMatch: async function (id, params){

      if (params != undefined && (params.txt != undefined || params.evalu != undefined)){
        let url = new URL("http://localhost:3000/data/" + id.toString());

        // Need to add user ID to params
        params["uid"] = localStorage.getItem('user');

        let res = await fetch(url,{
          method: 'PUT',
          headers: {
            "Content-Type": "application/json"
          },
          body: JSON.stringify(params)
        });
        let json = await res.json();
        return json;
      }
    },
    
    // Update the user's vote for match 'id' to 'vote'
    setVote: async function (id, vote) {


      let evalu = -1;
      if (vote == "yes"){
        evalu = 1;
      } else if (vote == "no"){
        evalu = 0;
      } else if (vote == "maybe"){
        evalu = 2;
      }

      let params = {
        evalu: evalu
      }
      let json = await this.updateMatch(id, params);
      if (json != undefined && json.statusCode != 200) {
        alert("Error updating evaluation for match " + id.toString() + " : change not saved");
      };
    },
    // Update users comment for mach 'id' to 'comment'
    setComment: async function (id, txt){
      let params = {
        txt: txt
      }
      let json = await this.updateMatch(id, params);
      if (json != undefined && json.statusCode != 200) {
        alert("Error updating comment for match " + id.toString() + " : change not saved");
      };
    },

    // Returns the number of rows (including blocks with blanks at end)
    getRowCount: function(){

      // page per block including blank spaces
       let pagePerBlock = Math.ceil(this.blockSize / this.perPage);
       let blanksPerBlock = pagePerBlock*this.perPage - this.blockSize;
       // how many blocks in the data
       let blockCount = Math.ceil(this.dataTotal / (this.blockSize));
       // how many are in last block
       let lastBlockCount = this.dataTotal % this.blockSize;

       // Calculate total number of rows
       return ((this.blockSize + blanksPerBlock) * (blockCount -1)) + lastBlockCount;
    },
    // Change display size of tiles, up to a limit
    // Keep display on page that contains the first tile on the page
    changeTileSize: function(arg){
      
      let pagePerBlock = Math.ceil(this.blockSize / this.perPage);
      let pageInBlock = ((this.displayPage - 1) % pagePerBlock) + 1; //block pages start at 1
      // The index of the first tile on the page
      let index = ((pageInBlock - 1)* this.perPage); //0 indexed

      if (arg == "increase" && this.tile.height < 500){
        this.tile.height += 50;
        this.tile.width += 50;
      } else if ( arg == "decrease" && this.tile.height > 100){
        this.tile.height -= 50;
        this.tile.width -= 50;
      }

      let newPagePerBlock = Math.ceil(this.blockSize / this.perPage);
      // Once adjusted, go to the page that contains that index
      let newPageInBlock = Math.floor(index / this.perPage) + 1; //pages start at 1
      let newDisplayPage = ((this.blockPage - 1) * newPagePerBlock) + newPageInBlock;
      
      this.displayPage = newDisplayPage;
    },
    // When the keystroke indicateds a page nav
    onPageNav: function(arg){
      if (arg == "next"){
        if (this.displayPage < this.pageCount){
          this.displayPage += 1;
        }
      } else if (arg == "prior"){
        if (this.displayPage > 1){
          this.displayPage -= 1;
        }
      }
    },

    // When vote is changed change cache and update database
    onChangeVote: async function(data){
      const vote = data.vote;
      const displayI = data.index; // Index in display page
      let pagePerBlock = Math.ceil(this.blockSize / this.perPage);
      let pageInBlock = this.displayPage % pagePerBlock;

      let blockI = (pageInBlock - 1)*this.perPage + displayI;

      // Change in display page: stats and vote
      let oldVote = this.dataList[blockI].user.vote;
      if (oldVote == "yes"){
        this.dataList[blockI].stats.yes -= 1;
      } else if (oldVote == "no") {
        this.dataList[blockI].stats.no -= 1;
      } else if (oldVote == "maybe"){
        this.dataList[blockI].stats.maybe -=1;
      }
      if (vote == "yes"){
        this.dataList[blockI].stats.yes += 1;
      } else if (vote == "no") {
        this.dataList[blockI].stats.no += 1;
      } else if (vote == "maybe"){
        this.dataList[blockI].stats.maybe +=1;
      }
      this.dataList[blockI].user.vote = vote;

      //change in database
      this.setVote(this.dataList[blockI].id, vote);

    },
    onChangeComment: async function(data){
        const comment = data.comment;
        const displayI = data.index; // Index in display page
        let pagePerBlock = Math.ceil(this.blockSize / this.perPage);
        let pageInBlock = this.displayPage % pagePerBlock;
        let blockI = (pageInBlock - 1)*this.perPage + displayI;

        // Change in page data
        this.dataList[blockI].user.comment = comment;
        // Change in database
        this.setComment(this.dataList[blockI].id, comment);

    },
    onTgtClick: async function(tgt){
      this.$emit("tgt-click", tgt);
    },
    onSrcClick: async function(src){
      this.$emit("src-click", src);
    }
  },
  watch: {
    // When display page changes see if you need to change block page
    displayPage: function () {
       let pagePerBlock = Math.ceil(this.blockSize / this.perPage);
       // If we're not in the correct block
       if (Math.ceil((this.displayPage) / pagePerBlock) != this.blockPage){
          // Blocks begin at 1, so must add 1
          this.blockPage = Math.floor((this.displayPage - 1)/pagePerBlock + 1);
       }
    }
  },
  created: function () {
    // this.setDisplayData();
  },
  mounted: function () {
    const vm = this;
    // Add listener for keystrokes
    window.addEventListener('keyup', function(event){  
      if(event.key == "+"){
        vm.changeTileSize("increase");
      } else if(event.key == "-"){
         vm.changeTileSize("decrease");
      }
    });
  }
}
</script>
<style scoped>
</style>