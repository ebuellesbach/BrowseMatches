const express = require('express');
const path = require('path');
const bodyParser = require('body-parser');
const session = require('express-session');
const jwt = require('jsonwebtoken');
const mysql = require('mysql2');
const mysqlssh = require('mysql-ssh');
const config = require('./config')
const fs = require('fs');

const app = express();
const log = true;


/*
* Database 
*/
const connection = mysql.createConnection(config.db_config);

/*
 *	App configuration
*/
var host = process.env.IP || '0.0.0.0';
var port = process.env.PORT || 3000;
var sessionSecret = process.env.SESSION_SECRET || 'e70a1e1ee4b8f662f78';

app.use("/js", express.static('../frontend/dist/js'));
app.use("/css", express.static('../frontend/dist/css'));
// So I can have image matches in public folder
app.use(express.static('public')); 

app.use(bodyParser.json());
app.use(bodyParser.urlencoded({
  extended: true
}));
app.use(express.json());

/*
 * For authentication: 
 * https://scotch.io/tutorials/vue-authentication-and-route-handling-using-vue-router
*/

// CORS middleware
const allowCrossDomain = function(req, res, next) {
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Methods', '*');
    res.header('Access-Control-Allow-Headers', '*');
    next();
}
app.use(allowCrossDomain)

app.post('/login', (request, response) =>{

	console.log("POST /login");
	
	let data = request.body;
	let sql = "SELECT * FROM `tblUsers` WHERE user=" + mysql.escape(data.user) + " AND email=" + mysql.escape(data.email) + ";";

	connection.query(sql , function(err, results, fields){
		if (err) response.send(err);
		else if (results.length != 1) {
			response.send({"statusCode": 401, "auth": false, token: null });
		} else {
			let token = jwt.sign({id: results[0].uid}, config.secret, { expiresIn: 86400});
			console.log("login success: " + results[0].user)
			response.send({"statusCode": 200, "auth": true, token: token, "user": results[0].uid, "username": results[0].user  });
		}

	});

})

/*
 *	Routing
*/

app.get('/', (request, response) => {
	
	if(log){ console.log('GET /'); }
	response.sendFile(path.resolve(__dirname + '/../frontend/dist/index.html'));
});

/*
 *	Database routes
*/



// Convert DB table format to front end format
function convertData(oldData, uid){

	// The oldData may have multiple results with the same pid but different uids
	// Process this: TO IMPROVE: do this in SQL
	let processData = [];
	let processIds = [];
	for (let i = 0; i < oldData.length; i++){
		
		// If we haven't encouted this entry yet, make one
		if (!processIds.includes(oldData[i].pid)){
			
			processIds.push(oldData[i].pid);

			let result = {}
			result["id"] = oldData[i].pid;
			result["imageUrl"] = "http://montaigu.cs.yale.edu:8088?matchid=" + oldData[i].pid.toString();
			result["tbldata"] = oldData[i];	
			
			result["user"] = {
					vote : "",
					comment: ""
				}
			result["otherComments"] = []		

			if (oldData[i].uid == uid){
				
				if (oldData[i].eval == 1){
					var vote = "yes";
				} else if (oldData[i].eval == 0){
					var vote = "no";
				} else if (oldData[i].eval == 2){
					var vote = "maybe";
				} else{
					var vote = "";
				}
				result["user"] = {
					vote : vote,
					comment: oldData[i].txt
				}
			} else {
				if (oldData[i].txt != undefined && oldData[i].txt != ""){
					result["otherComments"].push(oldData[i].txt)
				}
			}

			result["stats"] = {
				yes: 0,
				no: 0,
				maybe: 0
			}

			if (oldData[i].eval == 1){
				result["stats"].yes += 1;
			} else if (oldData[i].eval == 0){
				result["stats"].no += 1;
			}  else if (oldData[i].eval == 2){
				result["stats"].maybe += 1;
			}

			// To add to database
			result["confirmed"] = false;
			
			processData.push(result);
		}
		// Add a vote 
		else {

			let index = processIds.indexOf(oldData[i].pid);

			if (oldData[i].uid == uid){
				
				processData[index]["tbldata"] = oldData[i]; 
				
				if (oldData[i].eval == 1){
					var vote = "yes";
				} else if (oldData[i].eval == 0){
					var vote = "no";
				} else if (oldData[i].eval == 2){
					var vote = "maybe";
				} else{
					var vote = "";
				}
				processData[index]["user"] = {
					vote : vote,
					comment: oldData[i].txt
				}
			} else {
				if (oldData[i].txt != undefined && oldData[i].txt != ""){
					processData[index].push(oldData[i].txt)
				}
			}

			if (oldData[i].eval == 1){
				processData[index]["stats"].yes += 1;
			} else if (oldData[i].eval == 0){
				processData[index]["stats"].no += 1;
			}  else if (oldData[i].eval == 2){
				processData[index]["stats"].maybe += 1;
			}

		}
		
	}

	return processData;
}

// Return paginated data based on query
app.get('/data', (request, response) => {

	console.log("GET /data block:" + request.query.page);
	
	// Get params
	const uid = request.query.uid;
	const perPage = request.query.perPage;
	const page = request.query.page;
	let tgt = request.query.tgt;
    let src = request.query.src;
    const tgtLike = request.query.tgtLike;
    const srcLike = request.query.srcLike;
    const evalu = request.query.evalu;
    let txt = request.query.txt;
    const sortType = request.query.sortType;
    const sortOrder = request.query.sortOrder;

	/*
	 * Start query construction
	 */
	let sql = "SELECT * from `vBrowsematches` ";

	/*
	 * Set WHERE
	 */ 

	if (tgt != '' || src != '' || evalu != 'null' || txt != ''){
		sql += "WHERE ";

		let wheres = []
		if(tgt != ''){ 
			if(tgtLike == true){
				tgt = "%" + tgt + "%";
				wheres.push("tgt LIKE " + mysql.escape(tgt));
			} else {
				wheres.push("tgt= " + mysql.escape(tgt));
			}
		}
		if(src != ''){
			if(srcLike == true){
				src = "%" + src + "%";
				wheres.push("src LIKE " + mysql.escape(src));
			} else {
				wheres.push("src= " + mysql.escape(src));
			}
		}
		if(evalu != 'null'){ wheres.push("eval=" + mysql.escape(evalu));}
		if(txt != ''){ 
			txt = "%" + txt + "%";
			wheres.push("txt LIKE " + mysql.escape(txt));
		}
		sql += wheres.join(" AND ");
		sql += " ";
	}

	/*
	* Set ORDER
	*/ 

	// Default is probability desc
	let type = "probability";
	let order = "DESC"

	if (sortType == "error"){
		let order = "error";
	}
	if (sortOrder == "asc"){
		sortOrder = "ASC";
	}

	sql = sql + "ORDER BY " + type + " " + order + " ";
	
	/*
	 * Set LIMIT
	 */  

	// Where in the list data wanted starts (0 indexed)
	const start = perPage * (page - 1);

	sql = sql + "LIMIT " + start.toString() + "," + perPage.toString() + " ";

	/*
	 * End of query construction
	 */
	sql += ";";

	console.log(sql);
	
	// Execute the SQL query and send the response 
	connection.query(sql , function(err, results, fields){
		// Convert sql data to front end data
		const dataList = convertData(results, uid);

		let res = {
			err: err,
		 	page: page,  			// For record
		 	perPage: perPage, 		// For record
		 	list: dataList
		 };
		 response.send(res);
	});

});

// Return count of results in query 
app.get('/data/count', (request, response) => {

	console.log("GET /data/count");

	let tgt = request.query.tgt;
    let src = request.query.src;
    const tgtLike = request.query.tgtLike;
    const srcLike = request.query.srcLike;
    const evalu = request.query.evalu;
    let txt = request.query.txt;

	/*
	 * Start query construction
	 */
	let sql = "SELECT COUNT(*) AS count FROM `vBrowsematches` ";

	/*
	 * Set WHERE
	 */ 

	if (tgt != '' || src != '' || evalu != 'null' || txt != ''){
		sql += "WHERE ";

		let wheres = []
		if(tgt != ''){ 
			if(tgtLike == true){
				tgt = "%" + tgt + "%";
				wheres.push("tgt LIKE " + mysql.escape(tgt));
			} else {
				wheres.push("tgt= " + mysql.escape(tgt));
			}
		}
		if(src != ''){
			if(srcLike == true){
				src = "%" + src + "%";
				wheres.push("src LIKE " + mysql.escape(src));
			} else {
				wheres.push("src= " + mysql.escape(src));
			}
		}
		if(evalu != 'null'){ wheres.push("eval=" + mysql.escape(evalu));}
		if(txt != ''){ 
			txt = "%" + txt + "%";
			wheres.push("txt LIKE " + mysql.escape(txt));
		}
		sql += wheres.join(" AND ");
		sql += " ";
	}

	/*
	 * End of query construction
	 */
	sql += ";";
	
	// Execute the SQL query and send the response 
	connection.query(sql , function(err, results, fields){

		let res = {
			err: err,
			count: results[0].count
		 };
		 console.log("Result Count: " + results[0].count.toString());

		 response.send(res);
	});
});

// Return info from a particular match
app.get('/data/:matchId', (request, response) => {

	let id = request.params.matchId;
	const sql = 'SELECT * FROM `vBrowsematches` WHERE pid = ' + mysql.escape(id) + ' ;'
	connection.query(sql , function(err, results, fields){
		if (err) response.send(err);
		response.send(results);
	})

});

// Update a particular match eval
app.put('/data/:matchId', (request, response) => {

	console.log("PUT /data/" + request.params.matchId);
	
	// data will have the same info as the match
	let data = request.body;
	let id = request.params.matchId;

	// Update based on what was sent
	if (data != undefined && id != undefined){

		// check if the entry already exists 
		connection.query(
			"SELECT COUNT(*) AS count FROM `tblEvaluations` WHERE pid=" + mysql.escape(id) + " AND uid=" + mysql.escape(data.uid)+ ";"
		 , function(err, results, fields){

		 	if (err) response.send({"statusCode": 400});

		 	// if it exists, update
		 	else if (results[0].count > 0){
		 		let sql = "UPDATE `tblEvaluations` SET ";
				if (data.evalu != undefined){
					sql = sql + "eval = " + mysql.escape(data.evalu) + " ";
					if (data.txt != undefined){
						sql += ", ";
					}
				}
				if (data.txt != undefined){
					sql = sql + "txt = " + mysql.escape(data.txt) + " ";
				}

				sql = sql + "WHERE pid=" + mysql.escape(id);
				sql = sql + " AND uid=" + mysql.escape(data.uid) + ";"

				console.log(sql);

				connection.query(sql , function(err, results, fields){
					if (err) response.send({"statusCode": 400});
					else response.send({"statusCode": 200});
				});
		 	}
		 	// Otherwise insert
		 	else {

		 		let cols = [];
		 		let vals = [];

		 		cols.push('uid');
		 		vals.push(mysql.escape(data.uid))
		 		cols.push('pid');
		 		vals.push(mysql.escape(id))

		 		if (data.evalu != undefined){
		 			cols.push('eval');
		 			vals.push(mysql.escape(data.evalu));
		 		} else {
		 			cols.push('eval'); 
		 			vals.push(-1); // Needs to have a value
		 		}
		 		if (data.txt != undefined){
		 			cols.push('txt');
		 			vals.push(mysql.escape(data.txt));
		 		}
		 		let sql = "INSERT INTO `tblEvaluations` (";

		 		sql = sql + cols.join(",") + ")  VALUES (" + vals.join(",") + ");";

				console.log(sql);

				connection.query(sql , function(err, results, fields){
					if (err) response.send({"statusCode": 400});
					else response.send({"statusCode": 200});
				});

		 	}
		});
	} else {
		response.send({"statusCode": 400});
	}
	
});

app.listen(port, host, () => {
  console.log('Express running');
});
