import Vue from 'vue';
import App from './App.vue';
import router from './router';
import AsyncComputed from 'vue-async-computed'
import BootstrapVue from 'bootstrap-vue';
import 'bootstrap/dist/css/bootstrap.css'
import 'bootstrap-vue/dist/bootstrap-vue.css'

Vue.config.productionTip = false;

Vue.use(BootstrapVue);
Vue.use(AsyncComputed);

new Vue({
	data: {
		window: {
			width: 0,
			height: 0
		}
	},
	created() {
    	window.addEventListener('resize', this.handleResize)
    	this.handleResize();
	},
	mounted: function () {

	},
	destroyed() {
		window.removeEventListener('resize', this.handleResize)
	},
	methods: {
		handleResize() {
		    this.window.width = window.innerWidth;
		    this.window.height = window.innerHeight;
		}
	},
  	router,               
  	render: h => h(App)
}).$mount('#app')
