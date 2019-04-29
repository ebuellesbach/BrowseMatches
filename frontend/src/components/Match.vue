<template>
  <b-container>
    
    <!-- Row with Image -->
    <b-row>
      <!-- Image -->
      <b-col cols="12" lg="8" >
        <img :src="data.imageUrl" width="100%">
      </b-col>
      
      <!-- To right of image (or under if small) -->
      <b-col cols="12" lg="4">

        <!-- IDs -->
        <b-row cols="12">
          <b-col cols="12">
            <b> Target ID: </b> <span class="link" v-on:click="tgtClick()">
              {{data.tbldata.tgt}}
            </span>
            <br>
            <b> Source ID: </b> <span class="link" v-on:click="srcClick()">{{data.tbldata.src}} </span>
          </b-col>
        </b-row>
        
        <!-- Stats -->
        <b-row cols="12">
          <b-col cols="4" id="yes" class="stats" 
            v-bind:style="yesStyle"
            v-on:click="castVote('yes')"
            v-b-tooltip.hover title="Yes">
            {{data.stats.yes}}
            <img class="star" :id="'star-yes-' + data.id" src="star.png"> 
          </b-col>
          <b-col cols="4" id="maybe" class="stats" 
            v-bind:style="maybeStyle"
            v-on:click="castVote('maybe')"
            v-b-tooltip.hover title="Maybe">
            {{data.stats.maybe}}
            <img class="star" :id="'star-maybe-' + data.id" src="star.png"> 
          </b-col>
          <b-col cols="4" id="no" class="stats" 
            v-bind:style="noStyle"
            v-on:click="castVote('no')"
            v-b-tooltip.hover title="No">
            {{data.stats.no}}
            <img class="star" :id="'star-no-' + data.id" src="star.png"> 
          </b-col>
        </b-row>

        <!-- More info -->
        <b-row cols="12">
          <b-col cols="12">
            <b-container id="u-comment" v-b-tooltip.hover title="Your comment"
              v-bind:no-gutters="true">
              <b-row>
                <b-col cols="10" align-h="start" id="u-comment-interior">  
                  <span v-if="data.user.comment">
                    {{data.user.comment}} 
                  </span>
                  <span v-else>
                    No Comment
                  </span>
                </b-col>
                <b-col cols="2" align-h="end" align-v="center">
                  <b-button variant="light" id="comment-button" v-on:click="editComment()">
                    <img src="https://cdn0.iconfinder.com/data/icons/free-daily-icon-set/512/Comments-512.png" style="position:absolute;top:10%;left:10%;width:30px">
                  </b-button>
                </b-col>
              </b-row>
            </b-container>
          </b-col>
        </b-row>

      </b-col>
    </b-row>
    
    <b-row cols="12">
      <hr>
    </b-row>
    
    <!-- Row below image -->
    <b-row cols="12">
      <b-col>
        <b>Transformations: </b> <br>
        TX: {{data.tbldata.tx}} <br>
        TY: {{data.tbldata.ty}} <br>
        Theta: {{data.tbldata.theta}}
      </b-col>
      <b-col>
        <b> Evaluations: </b> <br>
        Error: {{data.tbldata.error}} <br>
        Probability: {{data.tbldata.probability}} <br>
      </b-col>
    </b-row>
    <b-row cols="12">
      <hr>
    </b-row>
    <b-row cols="12">
      <b> Other Comments: </b> <br>
      <ul>
        <li v-for="c in data.otherComments"> 
          {{c}}
        </li>
      </ul>
    </b-row>
  </b-container>
</template>

<script>
export default {
  name: 'Match',
  props: ['data', 'index'],
  data () {
    return {
      commentOpen: false
    }
  },
  computed: {
   
    // Conditional yes style:
    yesStyle: function() {

      var bColor = "white";
      var fontWeight = "normal";

      if (this.data.stats.yes > this.data.stats.no && this.data.stats.yes > this.data.stats.maybe){
        bColor = "#6ab110";
        fontWeight = "bold";
      }

      var style = {
        border: "2px solid " + bColor,
        fontWeight: fontWeight
      }

      return style;
    },
    // Conditional maybe style:
    maybeStyle: function() {

      var bColor = "white";
      var fontWeight = "normal";

      if (this.data.stats.maybe > this.data.stats.yes && this.data.stats.maybe > this.data.stats.no){
        bColor = "#f5dd00";
        fontWeight = "bold";
      }
      var style = {
      border: "2px solid " + bColor,
      fontWeight: fontWeight
      }
      return style;
    },
    // Conditional no style:
    noStyle: function() {

      var bColor = "white";
      var fontWeight = "normal";

      if (this.data.stats.no > this.data.stats.yes && this.data.stats.no > this.data.stats.maybe){
        bColor = "#dd1108";
        fontWeight = "bold";
      }
      var style = {
      border: "2px solid " + bColor,
      fontWeight: fontWeight
      }
      return style;
    }
  },
  methods: {
    tgtClick: function () {
      this.$emit('tgt-click', this.data.tbldata.tgt);

    },
    srcClick: function () {
      this.$emit('src-click', this.data.tbldata.src);

    },
    // Change the users vote on a match
    castVote: function (v) {

      // make all hidden
      this.hideStars();

      // If called to switch vote
      if (v != "") {
        
        // Local changes: Need to change on each level to immediatly change
        let oldVote = this.data.user.vote;
        if (oldVote == "yes"){
           this.data.stats.yes -= 1;
        } else if (oldVote == "no") {
           this.data.stats.no -= 1;
        } else if (oldVote == "maybe"){
           this.data.stats.maybe -=1;
        }
        if (v == "yes"){
           this.data.stats.yes += 1;
        } else if (v == "no") {
           this.data.stats.no += 1;
        } else if (v == "maybe"){
           this.data.stats.maybe +=1;
        }
        this.data.user.vote = v;
      
        // Push changes
        this.$emit('change-vote', {vote: v, index: this.index});
      }

      // Make asthetic change
      if (this.data.user.vote != ""){
        let id = "star-" + this.data.user.vote + "-" + this.data.id;
        document.getElementById(id).style.visibility = "visible"
      } 
    },
    hideStars: function() {
      // make all hidden
      document.getElementById("star-yes-" + this.data.id).style.visibility = "hidden";
      document.getElementById("star-maybe-" + this.data.id).style.visibility = "hidden";
      document.getElementById("star-no-" + this.data.id).style.visibility = "hidden";
    },
    // Allows user to edit their comment, clicked= if clicked vs key stroke
    editComment: function() {

      console.log("edit comment");
      
      // If editing, save upon button press
      if(this.commentOpen == true){
        let newComment = document.querySelector("#u-comment-interior").lastChild.value;
        if (newComment == null){
          newComment == "";
        }
        document.querySelector("#u-comment-interior").lastChild.remove();
        document.querySelector("#u-comment-interior").firstChild.style.visibility = 'visible';

        // Change local data
        this.data.user.comment = newComment;
        this.commentOpen = false;

        // Push up change
        this.$emit('change-comment', {comment: newComment, index: this.index});
      }
      // else, open up the text area
      else {

        // Turn the comment into a text input field
        let input = document.createElement("textarea");
        if (this.data.user.comment == undefined){
          input.value = "";
        } else {
          input.value = this.data.user.comment; // old comment
        }
        input.setAttribute("autofocus", true);
        input.style.width = "100%";
        input.style.height = "80px";
        input.style.topMargin = "2px";
        this.commentOpen = true;
        document.querySelector("#u-comment-interior").firstChild.style.visibility = 'hidden';
        document.querySelector("#u-comment-interior").append(input);
      }
    }
  },
  mounted: function () {
    this.castVote("", true);
  },
  updated: function () {
    this.hideStars();
    this.castVote("");
  }
}
</script>

<style scoped>

.star {
  position: absolute;
  top: 0%;
  right: 0%;
  visibility: hidden;
  width: 30%;
  max-width: 20px;
}

.stats {
  height: 40px;
  line-height: 35px;
  font-size: 120%;
  margin-top: 5px;
  margin-bottom: 5px;
}

#yes {
  background-color: #d6edba;
  border-radius: 5px;
}

#maybe {
  background-color: #f7f1c4;
  border-radius: 5px;
}

#no {
  background-color: #f7c6c4;
  border-radius: 5px;
}

#yes:hover, #no:hover, #maybe:hover {
  filter: brightness(90%);
}

#u-comment {
  /*position: :;*/
  border: 1px solid grey;
  width: 100%;
  min-height: 40px;
  margin: 5px;
  border-radius: 5px;
  font-size: 90%;
}

#comment-button {
  position: absolute;
  right: 5px;
  width: 90%;
  height: 90%;
  min-width: 35px;
  min-height: 35px;
  max-width: 40px;
  max-height: 40px;
  margin-top: 2px;
}

.link {
  color: green; 
  background-color: transparent; 
  text-decoration: none;
}

.link:visited {
  color: pink;
  background-color: transparent;
  text-decoration: none;
}

.link:hover {
  color: red;
  background-color: transparent;
  text-decoration: underline;
}

.link:active {
  color: yellow;
  background-color: transparent;
  text-decoration: underline;
}

</style>