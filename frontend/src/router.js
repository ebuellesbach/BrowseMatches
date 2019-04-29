import Vue from 'vue';
import VueRouter from 'vue-router';

import Welcome from '@/components/Welcome.vue';
import Overview from '@/components/Overview.vue';
import DisplayWrapper from '@/components/DisplayWrapper.vue';
import Match from '@/components/Match.vue';
import Login from '@/components/Login.vue';

Vue.use(VueRouter);

let router = new VueRouter({
	routes: [
		{ 
			path: '/',
			name: 'Welcome', 
			component: Welcome,
			props: true
		},
		{ 
			path: '/login',
			name: 'Login', 
			component: Login,
			props: true,
			meta: {
				guest: true
			}
		},
		{ 
			path: '/overview', 
			name: 'Overview',
			component: Overview,
			props: true,
			meta: {
				requiresAuth: true
			}
		},
		{ 
			path: '/matches', 
			name: 'DisplayWrapper',
			component: DisplayWrapper,
			props: true,
			meta: {
				requiresAuth: true
			}
		},
		{ 
			path: '/match', 
			name: 'Match',
			component: Match,
			props(route){
				return route.query || {}
			},
			meta: {
				requiresAuth: true
			}
		}
	]
});

//  SORUCE: https://scotch.io/tutorials/vue-authentication-and-route-handling-using-vue-router
router.beforeEach((to, from, next) => {
    if(to.matched.some(record => record.meta.requiresAuth)) {
        if (localStorage.getItem('jwt') == null) {
            next({
                path: '/login',
                params: { nextUrl: to.fullPath }
            });
        } else {
            next();
        }
    } else if(to.matched.some(record => record.meta.guest)) {
        next();
    }else {
        next();
    }
})

export default router;