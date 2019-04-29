<template>
	<div id="overall">
        <div id="header">
            <div>
                <h4>Login</h4>
            </div>
            <div>
                Note: Login only available to users currently in database
            </div>
        </div>
        <br>
        <b-form id="form">
            <b-form-group
              id="input-group-1"
              label="Username"
              label-for="input-1"
                >
              <b-form-input
                id="input-1"
                v-model="user"
                type="text"
              ></b-form-input>
            </b-form-group>
            <b-form-group
              id="input-group-2"
              label="Email:"
              label-for="input-2"
                >
              <b-form-input
                id="input-2"
                v-model="email"
                type="email"
              ></b-form-input>
            </b-form-group>
            <div>
            <b-button v-on:click="login()">
             Login
            </b-button>
        </b-form>
    </div>
</template>

<script>
    export default {
        data(){
            return {
                email : "",
                user : ""
            }
        },
        methods : {
            login: async function(e){

                if (this.email.length > 0 && this.user.length > 0) {

                	let url = new URL("http://localhost:3000/login");

			      	let res = await fetch(url,{
				        method: 'POST',
				        headers: {
				          "Content-Type": "application/json"
				        },
				        body: JSON.stringify({
	                        email: this.email,
	                        user: this.user
	                    })
				    });
				    let json = await res.json();

                    if (json == undefined || json.statusCode == 401){
                        alert("Invalid credentials");
                        this.email = "";
                        this.user = "";
                        document.querySelector("#input-1").innerHTML = "";
                        document.querySelector("#input-2").innerHTML = "";
                    }
                    else{
    				            localStorage.setItem('user',JSON.stringify(json.user));
                        localStorage.setItem('username',json.username);
                        localStorage.setItem('jwt',json.token);

                        if (localStorage.getItem('jwt') != null){
                            if(this.$route.params.nextUrl != null){
                                this.$router.push(this.$route.params.nextUrl);
                            }
                            else {
                                this.$router.push('Overview');
                            }
                        }  
                    }
                }
            }
        }
    }
</script>

<style>
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

<!-- SOURCE: https://scotch.io/tutorials/vue-authentication-and-route-handling-using-vue-router -->