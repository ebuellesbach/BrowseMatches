<template>
  <b-container fluid id="app">
    <b-row>
    <Header/>
    </b-row>
    <b-row>
    <router-view></router-view>
    </b-row>
    <b-row>
    <Footer/>
    </b-row> 
  </b-container>
</template>

<script>
import Header from './components/Header.vue';
import Footer from './components/Footer.vue';

export default {
  name: 'app',
  components: {
  Header,
  Footer
  }
}
</script>

<style>

#app {
  font-family: Verdana, Geneva, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;

  position: absolute;
  left: 0px;
  right: 10px;
  width: 100%;
  height: 100%;
  
}
</style>
