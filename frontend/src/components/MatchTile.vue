<template>
  <b-container fluid class=tile v-bind:style="tileStyle">
  	
  	<div :id="'tile-' + data.id">
  		<div id="click-area" @click="tileClick(data.id)"></div>
	  	<div id="match-stats">
		  	<div class="yes" v-bind:style="yesStyle" v-on:click="castVote('yes')"> 
		  		<div class="stat"> 
		  			{{data.stats.yes}}
		  		</div> 
		  		<img class="star":id="'star-yes-' + data.id" src="star.png"> 
		  	</div>
		  	<div class="maybe" v-bind:style="maybeStyle" v-on:click="castVote('maybe')">
		  		<div class="stat"> 
		  			{{data.stats.maybe}} 
		  		</div>
		  		<img class="star" :id="'star-maybe-' + data.id" src="star.png"> 
		  	</div>
		  	<div class="no" v-bind:style="noStyle" v-on:click="castVote('no')">
		  		<div class="stat">
		  			{{data.stats.no}} 
		  		</div>
		  		<img class="star" :id="'star-no-' + data.id" src="star.png"> 
		  	</div>
	  	</div>

	 </div>

  </b-container>
</template>

<script>
export default {
  name: 'MatchTile',
  /*
		data:
		- id
		- confirmed
		- imageUrl
		- stats
			- yes
			- no
			- maybe
  */
  props: ['data', 'tile', 'selectedTile', 'index'],
  data: function () {
  	return {

  	}
  },
  computed: {
  	// Conditional tile styleing
  	tileStyle: function () {
  		var h = this.tile.height.toString() + "px";
  		var w = this.tile.width.toString() + "px";
  		var m = this.tile.margin.toString() + "px";
  		var i = "url('" + this.data.imageUrl + "')";

  		// A tile of 200 height has a 100% font size
  		var f = (this.tile.height/2).toString() + "%";

  		var style = {
  			height: h,
  			width: w,
  			margin: m,
  			backgroundImage: i,
  			fontSize: f,
  			backgroundRepeat: "no-repeat",
  			backgroundSize: "95%",
  			backgroundAttatchment: "fixed",
  			backgroundPosition: "center center",
  			backgroundColor: "lightgray",
        borderRadius: "5px"
  		};

  		// If clicked add a border
  		if (this.data.confirmed){
  			style.border = "4px solid #ffbf80"
  		} else {
  			style.border = "4px solid white"
  		}
  		if (this.selectedTile == this.data.id){
  			style.boxShadow = "0px 0px 5px 8px grey"
  		}

  		return style;
  	},
  	// Conditional yes style:
  	yesStyle: function() {

  		var bColor = "lightgray";
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

  		var bColor = "lightgray";
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

  		var bColor = "lightgray";
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
  	tileClick: function () {
  		this.$emit('clicked', this.index);
  	}

  },
  mounted: function () {
  	this.castVote("");
  },
  updated: function () {
 	  this.hideStars();
 	  this.castVote(""); 
  }

}
</script>

<style scoped>

/* Full tile*/
.tile{
	position: relative; 
  border-radius: 5px;
}

/* Create clickable area that is not clicable on stats*/
#click-area {
	position: absolute;
	top:0px;
	width: 100%;
	height: 85%;
	left:0%;
  top: 0%;
}

/* Element with match ID*/
#match-label {
	position: absolute;
	left: 0px;
	top: 0px;
	width: 100%;
	height: 15%;
	background-color: lightgrey;
	border: 1px solid grey;
  border-radius: 5px;
}

/* Element with yes/no/maybe */
#match-stats {
	position: absolute;
	left: 0px;
	top: 0px;
	width: 15%;
	height: 60%;
}

.star {
	position: absolute;
	top: -5%;
	right: -5%;
	visibility: hidden;
  width: 35%;
}

.yes {
	position: absolute;
	top: 0%;
	height: 33%;
	width: 100%;
	background-color: #d6edba; 
  border-radius: 5px;
}

.maybe {
	position: absolute;
	top: 33%;
	height: 34%;
	width: 100%;
	background-color: #f7f1c4;
  border-radius: 5px;
}

.no {
	position: absolute;
	top: 67%;
	height: 33%;
	width: 100%;
	background-color: #f7c6c4;
  border-radius: 5px;
}

.yes:hover, .no:hover, .maybe:hover {
  filter: brightness(90%);
}


.stat {
	position: relative;
	top: 50%;
	transform: translateY(-50%);
}

</style>