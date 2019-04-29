-- webmatches.sql
-- Benedict Brown
--
-- Schema for storing matches in a database
--
-- based on design by Judy Trinh and Lucy Xiao

-- disable foreign key constraints while rebuilding the table structure
SET FOREIGN_KEY_CHECKS = 0;

-- list of matchers (e.g. ribbonmatcher, patchcmatcher, ...)
SELECT 'tblMatchers';
DROP TABLE IF EXISTS tblMatchers;
CREATE TABLE tblMatchers (
   mid      INT NOT NULL AUTO_INCREMENT,  -- matcher id
   matcher  VARCHAR(128) NOT NULL,        -- matcher name

   PRIMARY KEY(mid)
);



-- list of users
SELECT 'tblUsers';
DROP TABLE IF EXISTS tblUsers;
CREATE TABLE tblUsers (
   uid      INT NOT NULL AUTO_INCREMENT,  -- user id
   user     VARCHAR(128) NOT NULL,        -- user name
   email    VARCHAR(128) NOT NULL,        -- user e-mail address

   PRIMARY KEY(uid)
);



-- list of potential match attributes
SELECT 'tblAttributes';
DROP TABLE IF EXISTS tblAttributes;
CREATE TABLE tblAttributes (
   aid      INT NOT NULL AUTO_INCREMENT,    -- attribute id
   attrName VARCHAR(128) NOT NULL,          -- attribute name
   attrType INT NOT NULL,                   -- attribute type
   attrMin  FLOAT NOT NULL DEFAULT 0,       -- minimum allowed value
   attrMax  FLOAT NOT NULL DEFAULT 1,       -- maximum allowed value

   isDesc   BOOLEAN NOT NULL DEFAULT FALSE, -- are lower scores better?
   cull     BOOLEAN NOT NULL DEFAULT FALSE, -- cull this attribute?
   visible  BOOLEAN NOT NULL DEFAULT TRUE,  -- display this attribute
   classify BOOLEAN NOT NULL DEFAULT TRUE,  -- use this attrib in decision tree?

   PRIMARY KEY(aid)
);



-- list of match proposals
SELECT 'tblProposals';
DROP TABLE IF EXISTS tblProposals;
CREATE TABLE tblProposals (
   pid      INT NOT NULL AUTO_INCREMENT,     -- proposal id

   -- matcher ids go on the attribute, not the proposal
   -- because the proposal can combine attributes from
   -- multiple matchers
   -- mid      INT NOT NULL,                 -- matcher id


   tgt      VARCHAR(128) NOT NULL,           -- target fragment
   src      VARCHAR(128) NOT NULL,           -- source fragment
   theta    FLOAT NOT NULL,                  -- alignment rotation
   tx       FLOAT NOT NULL,                  -- alignment translation (x)
   ty       FLOAT NOT NULL,                  -- alignment translation (y)

   error       FLOAT NOT NULL DEFAULT  1000, -- ribbon error (set by trigger)
   probability FLOAT NOT NULL DEFAULT -1000, -- patch matcher probability (set by trigger)

   PRIMARY KEY(pid),
   INDEX(tgt),
   INDEX(src),
   INDEX(error),
   INDEX(probability)
   -- FOREIGN KEY(mid) REFERENCES tblMatchers(mid)
);



-- list of proposal attributes
SELECT 'tblProposalAttributes';
DROP TABLE IF EXISTS tblProposalAttributes;
CREATE TABLE tblProposalAttributes (
   paid     INT NOT NULL AUTO_INCREMENT,    -- unique id
   pid      INT NOT NULL,                   -- proposal id
   aid      INT NOT NULL,                   -- attribute id
   mid      INT NOT NULL,                   -- matcher id
   val      FLOAT NOT NULL,                 -- attribute value
   txt      VARCHAR(512),                   -- attribute text (e.g. contents of xf attrib)

   PRIMARY KEY(paid),

   FOREIGN KEY (pid) REFERENCES tblProposals(pid),
   FOREIGN KEY (aid) REFERENCES tblAttributes(aid),
   FOREIGN KEY (mid) REFERENCES tblMatchers(mid),

   INDEX(pid),
   INDEX(aid),
   INDEX(val)
);



-- list of proposal evaluations
SELECT 'tblEvaluations';
DROP TABLE IF EXISTS tblEvaluations;
CREATE TABLE tblEvaluations (
   pid      INT NOT NULL,                      -- proposal id
   uid      INT NOT NULL,                      -- user id

   eval     FLOAT NOT NULL,                    -- evaulation (0 = bad, 1 = good, -1 = UNKNWON)
   txt      VARCHAR(256),                      -- comment

   tstamp   TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP, -- evaluation timestamp

   PRIMARY KEY (pid, uid),
   FOREIGN KEY (pid) REFERENCES tblProposals(pid),
   FOREIGN KEY (uid) REFERENCES tblUsers(uid)
);


-- create attribute entries
insert into tblAttributes (attrType, isDesc, attrName) values (0, 0, 'error');
insert into tblAttributes (attrType, isDesc, attrName) values (0, 0, 'old_volume');
insert into tblAttributes (attrType, isDesc, attrName) values (0, 0, 'volume');
insert into tblAttributes (attrType, isDesc, attrName) values (0, 0, 'overlap');
insert into tblAttributes (attrType, isDesc, attrName) values (0, 0, 'xf');
insert into tblAttributes (attrType, isDesc, attrName) values (0, 1, 'Probability');


-- create views for Browsematches
SELECT 'vProposalVolumes';
CREATE OR REPLACE VIEW vProposalVolumes AS
SELECT pid, MIN(val) AS volume FROM tblProposalAttributes WHERE aid = 3 GROUP BY pid;

SELECT 'vProposalEvals';
CREATE OR REPLACE VIEW vProposalEvals AS
SELECT pid, uid, IFNULL(eval, -1) AS eval, IFNULL(txt, '') AS txt, tstamp
FROM tblProposals NATURAL LEFT JOIN tblEvaluations;

SELECT 'vBrowsematches';
CREATE OR REPLACE VIEW vBrowsematches AS
SELECT * FROM     tblProposals
-- NATURAL LEFT JOIN vProposalVolumes
NATURAL LEFT JOIN vProposalEvals;

-- re-enable foreign key checks
SET FOREIGN_KEY_CHECKS = 1;

-- debug table
SELECT 'tblDebug';
DROP TABLE IF EXISTS tblDebug;
CREATE TABLE tblDebug (
    id INT NOT NULL AUTO_INCREMENT,
    txt TEXT,

    PRIMARY KEY(id)
);

-- function to update error and probability so we can access them efficiently
SELECT 'procUpdateSortKeysHelper';
DROP PROCEDURE IF EXISTS procUpdateSortKeysHelper;
DELIMiTER //
CREATE PROCEDURE procUpdateSortKeysHelper(IN rpid INT, IN rminerr FLOAT, IN rmaxprob FLOAT)
proc: BEGIN
   DECLARE xf VARCHAR(256) DEFAULT NULL;

   -- update error and probability
   UPDATE tblProposals SET error = rminerr, probability = rmaxprob WHERE pid = rpid;

   -- update transformation to the one with the smallest error if error is available, otherwise the highest probability
   -- INSERT INTO tblDebug SET txt=rpid;
   -- INSERT INTO tblDebug SET txt=rminerr;
   -- INSERT INTO tblDebug SET txt=rmaxprob;

   SELECT NULL INTO xf;
   SELECT TRIM(txt) FROM tblProposalAttributes WHERE pid = rpid AND aid = 1 AND val = rminerr LIMIT 1 INTO xf; -- get xf from error
   IF xf IS NULL THEN
      SELECT TRIM(txt) FROM tblProposalAttributes WHERE pid = rpid AND aid = 5 AND val = rmaxprob LIMIT 1 INTO xf; -- no luck, try Probability
   END IF;

   -- INSERT INTO tblDebug SET txt=xf;

   IF xf IS NULL THEN
      UPDATE tblProposals SET error=rminerr, probability=rmaxprob WHERE pid=rpid;
   ELSE
      UPDATE tblProposals
      SET    error=rminerr, probability=rmaxprob,
             theta=ATAN2(-SUBSTRING_INDEX(SUBSTRING_INDEX(xf, ' ', 2), ' ', -1), SUBSTRING_INDEX(xf, ' ', 1)),
             tx=SUBSTRING_INDEX(SUBSTRING_INDEX(xf, ' ', 4), ' ', -1),
             ty=SUBSTRING_INDEX(SUBSTRING_INDEX(xf, ' ', 8), ' ', -1)
      WHERE pid=rpid;
   END IF;
END//
DELIMITER ;

SELECT 'procUpdateSortKeys';
DROP PROCEDURE IF EXISTS procUpdateSortKeys;
DELIMITER //
CREATE PROCEDURE procUpdateSortKeys(IN rpid INT)
proc: BEGIN
    DECLARE minerr  FLOAT DEFAULT  1000;
    DECLARE maxprob FLOAT DEFAULT -1000;
    SELECT IFNULL(MIN(val), minerr)  INTO minerr  FROM tblProposalAttributes WHERE pid = rpid AND aid = 1;
    SELECT IFNULL(MAX(val), maxprob) INTO maxprob FROM tblProposalAttributes WHERE pid = rpid AND aid = 5;
    CALL procUpdateSortKeysHelper(rpid, minerr, maxprob);
END//
DELIMITER ;

-- update all sort keys all at once because the triggers seem to be way to slow
SELECT 'procUpdateAllSortKeys';
DROP PROCEDURE IF EXISTS procUpdateAllSortKeys;
DELIMITER //
CREATE PROCEDURE procUpdateAllSortKeys()
proc: BEGIN
   DECLARE rpid INT DEFAULT NULL;
   DECLARE done INT DEFAULT FALSE;
   DECLARE rminerr, rmaxprob FLOAT;

   DECLARE cur1 CURSOR FOR SELECT pid, minerr, maxprob
                          FROM   tblProposals
                          NATURAL LEFT JOIN (SELECT pid, IFNULL(MIN(val),  1000) AS minerr  FROM tblProposalAttributes WHERE aid = 1 GROUP BY pid) AS vError
                          NATURAL LEFT JOIN (SELECT pid, IFNULL(MAX(val), -1000) AS maxprob FROM tblProposalAttributes WHERE aid = 5 GROUP BY pid) AS vProb;

   DECLARE CONTINUE HANDLER FOR NOT FOUND SET done = TRUE;

   OPEN cur1;
   row_loop: LOOP
      IF done THEN LEAVE row_loop; END IF;
      FETCH cur1 INTO rpid, rminerr, rmaxprob;
      CALL procUpdateSortKeysHelper(rpid, rminerr, rmaxprob);
   END LOOP;

   CLOSE cur1;
END//
DELIMITER ;

-- attribute triggers
-- SELECT 'triggerAttributeInsert';
-- DROP TRIGGER IF EXISTS triggerAttributeInsert;
-- DELIMITER //
-- CREATE TRIGGER triggerAttributeInsert AFTER INSERT ON tblProposalAttributes
-- FOR EACH ROW CALL procUpdateSortKeys(NEW.pid); //
-- DELIMITER ;

-- SELECT 'triggerAttributeUpdate';
-- DROP TRIGGER IF EXISTS triggerAttributeUpdate;
-- DELIMITER //
-- CREATE TRIGGER triggerAttributeUpdate AFTER UPDATE ON tblProposalAttributes
-- FOR EACH ROW CALL procUpdateSortKeys(NEW.pid); //
-- DELIMITER ;

-- SELECT 'triggerAttributeDelete';
-- DROP TRIGGER IF EXISTS triggerAttributeDelete;
-- DELIMITER //
-- CREATE TRIGGER triggerAttributeDelete AFTER DELETE ON tblProposalAttributes
-- FOR EACH ROW CALL procUpdateSortKeys(OLD.pid); //
-- DELIMITER ;

-- utiliy functions/procedures
SELECT 'procAttr';
DROP PROCEDURE IF EXISTS procAttr;
DELIMITER //
CREATE PROCEDURE procAttr(IN p INT, IN a INT, IN m INT, IN v FLOAT, IN t VARCHAR(256))
proc: BEGIN
    REPLACE INTO tblProposalAttributes (pid, aid, mid, val, txt) VALUES (p, a, m, v, t);
END//
DELIMITER ;

SELECT 'fnUser';
DROP FUNCTION IF EXISTS fnUser;
DELIMITER //
CREATE FUNCTION fnUser(user_ VARCHAR(256))
RETURNS INT
proc: BEGIN
  DECLARE newuid INT DEFAULT NULL;
  SELECT uid INTO newuid FROM tblUsers WHERE user = user_ LIMIT 1;
  IF newuid IS NULL THEN
      INSERT INTO tblUsers (user, email) VALUES (user_, '');
      SELECT LAST_INSERT_ID() INTO newuid;
  END IF;

  RETURN newuid;
END//
DELIMITER ;

SELECT 'fnMatcher';
DROP FUNCTION IF EXISTS fnMatcher;
DELIMITER //
CREATE FUNCTION fnMatcher(matcher_ VARCHAR(256))
RETURNS INT
proc: BEGIN
  DECLARE newmid INT DEFAULT NULL;
  SELECT mid INTO newmid FROM tblMatchers WHERE matcher = matcher_ LIMIT 1;
  IF newmid IS NULL THEN
      INSERT INTO tblMatchers (matcher) VALUES (matcher_);
      SELECT LAST_INSERT_ID() INTO newmid;
  END IF;

  RETURN newmid;
END//
DELIMITER ;

SELECT 'fnProposal';
DROP FUNCTION IF EXISTS fnProposal;
DELIMITER //
CREATE FUNCTION fnProposal(tgt_ VARCHAR(256), src_ VARCHAR(256), theta_ FLOAT, tx_ FLOAT, ty_ FLOAT)
RETURNS INT
proc: BEGIN
  DECLARE newpid INT DEFAULT NULL;
  SELECT   pid  INTO newpid FROM tblProposals
  WHERE    src = src_ AND tgt = tgt_ AND (57.295779513 * ABS(theta - theta_)) < 5 AND (ABS(tx - tx_) + ABS(ty - ty_)) < 5
  ORDER BY 57.295779513 * ABS(theta - theta_) + ABS(tx - tx_) + ABS(ty - ty_)
  LIMIT 1;

  IF newpid IS NULL THEN
      INSERT INTO tblProposals (tgt, src, theta, tx, ty) VALUES (tgt_, src_, theta_, tx_, ty_);
      SELECT LAST_INSERT_ID() INTO newpid;
  END IF;

  RETURN newpid;
END//
DELIMITER ;
