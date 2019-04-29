<template>
  <b-container fluid >

    <!-- Modal that displays when tile is clicked -->
    <b-modal v-if="modal.show" hide-footer v-model="modal.show" size="lg"> 
      <template slot="modal-title">
          {{modal.title}}
      </template>
      <Match v-if="modal.id" 
        :data="data[modal.index]" 
        :index="modal.index"
        @change-vote="onChangeVote"
        @change-comment="onChangeComment"
        @tgt-click="onTgtClick"
        @src-click="onSrcClick"/>
    </b-modal>
  	
    <!-- Tile display -->
    <b-row v-for="i in rows" class="no-gutters align-items-center" align-h="center">
  		
      <b-col v-for="j in cols" v-if="data[(i-1)*cols + j - 1]" class="no-gutters align-items-center">
        
        <!-- Internal tile display  -->
        <MatchTile :data="data[(i-1)*cols + j - 1]"
                   :tile="tile"
                   :index="(i-1)*cols + j - 1"
                   :selectedTile="selectedId"
                   @clicked="onTileClick"
                   @change-vote="onChangeVote"/>

  		</b-col>
  	</b-row>
  </b-container>
</template>

<script>
import MatchTile from './MatchTile.vue';
import Match from './Match.vue';

export default {
  name: 'Page',
 /* 
        data: List of match objects {}
        tile: {
            height
            width
            margin
        }
        rows, cols: how many tiles to display
  */
  props: ['data', 'tile', 'rows', 'cols'],
  components: {
    MatchTile,
    Match
  },
  data () {
  	return {
      selectedIndex: 0,
      modal: {
        show: false,
        title: "",
        id: "",
        index: 0,
      }
  	}
  },
  computed: {
    selectedId: function () {
      if (this.data[this.selectedIndex] != undefined){
        return this.data[this.selectedIndex].id;
      } else {
        return "";
      }
    }
  },
  methods: {

    onTgtClick: function(tgt){
      this.modal.show = false;
      this.$emit("tgt-click", tgt);
    },
    onSrcClick: function(src){
      this.$emit("src-click", src);
    },
    // Display the modal with information
    onTileClick: function(index){
      this.modal.title = "Match: " + this.data[index].id;
      this.modal.id = this.data[index].id; 
      this.modal.show = true;
      this.modal.index = index;
      this.selectedIndex = index;

    },
    // When the vote happens change in page and emit to change in database
    onChangeVote: function(data){
      const vote = data.vote;
      const index = data.index;

      // Change stats and vote: need to change on all levels 
      let oldVote = this.data[index].user.vote
      if (oldVote == "yes"){
        this.data[index].stats.yes -= 1;
      } else if (oldVote == "no") {
        this.data[index].stats.no -= 1;
      } else if (oldVote == "maybe"){
        this.data[index].stats.maybe -=1;
      }
      if (vote == "yes"){
        this.data[index].stats.yes += 1;
      } else if (vote == "no") {
        this.data[index].stats.no += 1;
      } else if (vote == "maybe"){
        this.data[index].stats.maybe +=1;
      }
      this.data[index].user.vote = vote;
      
      // Push change up
      this.$emit("change-vote", data);

    },
    onChangeComment: function(data){
      this.data[data.index].user.comment = data.comment;
      this.$emit("change-comment", data);

    },

    // Keystrokes for navigation
    upArrowPressed: function () {
      if (this.selectedIndex > this.cols - 1){
        this.selectedIndex -= this.cols;
        //if modal open change it
        if (this.modal.show){
          this.onTileClick(this.selectedIndex);
        }
      }
    },
    downArrowPressed: function () {
      if (this.selectedIndex <  (this.rows * this.cols) - this.cols){
        this.selectedIndex += this.cols;
           //if modal open change it
        if (this.modal.show){
          this.onTileClick(this.selectedIndex);
        }
      }
    },
    rightArrowPressed: function () {
      if (this.selectedIndex < this.rows*this.cols-1){
        this.selectedIndex +=1;
        //if modal open change it
        if (this.modal.show){
          this.onTileClick(this.selectedIndex);
        }
      } else {
        this.selectedIndex = 0;
        this.$emit('key-page-nav', 'next');
        // If moving to new page close modal
        if (this.modal.show){
          this.modal.show = false;
        }
      }
      
    },
    leftArrowPressed: function () {
      if (this.selectedIndex > 0){
        this.selectedIndex -= 1;
        //if modal open change it
        if (this.modal.show){
          this.onTileClick(this.selectedIndex);
        }
      } else {
        this.selectedIndex = this.data.length - 1;
        this.$emit('key-page-nav', 'prior');
        // If moving to new page close modal
        if (this.modal.show){
          this.modal.show = false;
        }

      }

    },
  },
  mounted: function () {

    var vm = this;

    // Add listener for keystrokes
    window.addEventListener('keyup', function(event){  
      if(event.key == "ArrowDown"){
        vm.downArrowPressed();
      } else if(event.key == "ArrowUp"){
        vm.upArrowPressed();
      } else if(event.key == "ArrowRight"){
        vm.rightArrowPressed();
      } else if(event.key == "ArrowLeft"){
        vm.leftArrowPressed();
      } else if(event.key == " "){
          // On space bar: open modal for selected tile
        vm.onTileClick(vm.selectedIndex);
      } else if(event.key == "1"){
        vm.onChangeVote({vote: "yes", index: vm.selectedIndex});
      } else if(event.key == "2"){
        vm.onChangeVote({vote: "maybe", index: vm.selectedIndex});
      } else if(event.key == "3"){
        vm.onChangeVote({vote: "no", index: vm.selectedIndex});
      }
    });
  }
}
</script>
<style scoped>
</style>