<template>
  <b-container fluid id="nav">
    <b-navbar toggleable="md" type="dark" variant="info" :fixed="true">

      <b-navbar-brand to="/">
        BrowseMatches
      </b-navbar-brand>
      <b-navbar-nav >
        <b-nav-item to="/overview">Search</b-nav-item>
      </b-navbar-nav>
      <b-navbar-nav >
        <b-nav-item to="/" @click="logout">Logout</b-nav-item>
      </b-navbar-nav>
      <b-navbar-nav>
        <b-nav-item to="/login">Login</b-nav-item>
      </b-navbar-nav>


    </b-navbar>
</b-container>
</template>

<script>
export default {
  name: 'Header',
  computed: {
    hello: function(){
      let uname = localStorage.getItem('username');
      if (uname != undefined){
        return "Hello, " + uname + "!";
      } else {
        return "Hello!";
      }
    }
  },
  methods: {
    logout: function() {
        localStorage.clear();
    }
  }
}
</script>

<style scoped>
#nav {
  background-color: #24A2B8;
}
</style>
