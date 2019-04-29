#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <QtGui>
#include <QApplication>
#include <QDebug>
#include <QLocale>
#include <QTranslator>
#include <QMenu>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>

#include "Database.h"
#define SETTINGS_DB_ROOT_KEY "db/root"

#include "RibbonMatcher.h"

#include "XF.h"
#include "TabletopModel.h"
#include "TabletopIO.h"

#include "browsematches.h"

#define ARRIVED_HERE fprintf(stderr, "Arrived here: %s:%d\n", __FILE__, __LINE__)

static bool  useDB = false;

const  char *thera::matchBrowser::statusNames[] = { "Unknown", "Yes", "Maybe", "No", "Conflict"};

QNetworkAccessManager thera::thumbLabel::nam;

static void usage(FILE *out) {
  // fprintf(out, "usage: theraprocess [ (-h|--help) ] [ (-l|--locale) <locale> ] <db-base-path>\n");
  exit(out == stderr);
}

int main(int argc, char **argv) {
    Q_INIT_RESOURCE(theragui);

#ifdef Q_WS_MAX
    QCoreApplication::setOrganizationName("princeton.edu"); // Mac OS X requires URL instead of name
#else
    QCoreApplication::setOrganizationName("Princeton");
#endif
    QCoreApplication::setOrganizationDomain("princeton.edu");
    QCoreApplication::setApplicationName("browsematches");

    QSettings settings;
  
    QApplication app(argc, argv);

    // Parse arguments (from griphos/main.cc)
    //
    QString locale = QLocale::system().name();  // e.g. "en_US"
    QString dbDir;
    QDir    thumbDir;

    float scale = 0.5f;
    int rows = 4, cols = 5;
    QString sortField("probability");
    bool sortLT = true;

    fprintf(stderr, "dbDir.size() == %d\n", (int) dbDir.size());
    for (int i = 1; i < argc; i++) {
      if (argv[i][0] == '-') {
        if (!strcmp(argv[i], "-h") ||
            !strcmp(argv[i], "--help")) {
          usage(stdout);
        } else if (!strcmp(argv[i], "-l") ||
                   !strcmp(argv[i], "--locale")) {
          if (++i < argc) locale = argv[i];
          else usage(stderr);
        } else if (!strcmp(argv[i], "--thumbDir")) {
          if (++i < argc) {
        	  thumbDir = argv[i];
        	  qDebug() << "thumbDir set to " << thumbDir;
          }
        } else if (!strcmp(argv[i], "-s") ||
                   !strcmp(argv[i], "--scale")) {
          if (++i < argc) scale = atof(argv[i]);
          else usage(stderr);
        } else if (!strcmp(argv[i], "-g") ||
                   !strcmp(argv[i], "--geom")) {
          if (++i < argc) rows = atoi(argv[i]);
          else usage(stderr);
          if (++i < argc) cols = atoi(argv[i]);
          else usage(stderr);
        } else if (!strcmp(argv[i], "--sort")) {
          if (++i < argc) sortField = argv[i];
          else usage(stderr);
          if (++i < argc) sortLT = atoi(argv[i]);
          else usage(stderr);
        } else if (!strcmp(argv[i], "--usedb")) {
          useDB = true;
        } else usage(stderr);
      } else if (!dbDir.size()) {
        fprintf(stderr, "Setting dbDir.\n");
        dbDir = argv[i];
      } else usage(stderr);
    }

    // Set up translator(s) (from griphos/main.cc)
    //
    QTranslator translator;
    if (locale != "C" && !locale.startsWith("en")) {
      if (translator.load(QString(":/rcc/localization/locale_") + locale)) {
        qDebug("using system localization `%s'", qPrintable(locale));
        app.installTranslator(&translator);
      } else
        qDebug("no appropriate localization for locale `%s'", qPrintable(locale));
    }

    thera::matchBrowser *mb = new thera::matchBrowser(dbDir, thumbDir, NULL, rows, cols, scale, sortField, sortLT, useDB);
    mb->show();

    int rc = app.exec();
    return rc;
}

#if 0
static inline bool sortFCErr(const thera::FragmentConf &fc1, const thera::FragmentConf &fc2) {
  return fc1.getDouble("error") < fc2.getDouble("error");
}
#else
struct sortFCClass {
  QString field;
  bool lt;

  sortFCClass(QString field_, bool lt_) : field(field_), lt(lt_) {}
  bool operator () (const thera::FragmentConf &fc1, const thera::FragmentConf &fc2) const {
    double d1 = fc1.getDouble(field);
    double d2 = fc2.getDouble(field);

    // bool rc = lt ? (((d2 == 0) && (d1 > 0)) || (d2 > d1)) : (d1 > d2);
    bool rc = (d1 > 0) && ((d2 <= 0) || (d2 > d1));
    // fprintf(stderr, "%d %d %d %g %g %d\n", lt, d1 == 0, d2 == 0, d1, d2, rc);
    return rc;
  }
};

struct sortFCIdxClass {
  const sortFCClass &sfc;
  const std::vector<thera::FragmentConf> &fc;

  sortFCIdxClass(const sortFCClass &s, const std::vector<thera::FragmentConf> &f) : sfc(s), fc(f) {}
  bool operator () (int a, int b) const {
    return sfc(fc[a], fc[b]);
  }
};
#endif

static QString thumbName(thera::FragmentConf &conf) {
  if (useDB) {
    return QString("http://hoei.cis.upenn.edu:8088?matchid=") + conf.getString("id").trimmed();
  } else {
    thera::FragmentRef target(conf.mFragments[thera::FragmentConf::TARGET]);
    thera::FragmentRef source(conf.mFragments[thera::FragmentConf::SOURCE]);
    if (target.isNil() || source.isNil()) return QString();
    if (target.id() > source.id()) {
      conf.swapSourceAndTarget();
      std::swap(source, target);
    }

    double error = conf.getDouble("error");
    double volume = conf.getDouble("volume");
    if (error == 0 && volume == 0) error = conf.getDouble("Probability");
    return QString("%3_%1_%2_%4.jpg").arg(target.id(), source.id(),
                                          QString::number(error, 'f', 4),
                                          QString::number(volume, 'f', 4));
  }
}

void thera::matchBrowser::reloadMatches() {
  // clear the existing list of fragmentConfs and the idmap
  ARRIVED_HERE;
  fc.clear();
  ARRIVED_HERE;
  idmap.clear();

  if (useDB) {
    // find out how many records there are
    ARRIVED_HERE;
    QSqlQuery query;
    ARRIVED_HERE;
    query.exec("SELECT COUNT(pid) FROM tblProposals;");
    ARRIVED_HERE;
    query.next();
    num_matches = query.value(0).toInt();
    ARRIVED_HERE;
    qDebug() << "num_matches: " << num_matches;
  } else {
    // load from XML file
    QTime stopwatch; // do a little benchmarking
    stopwatch.start();

    FragmentQueryResult fqr = RibbonMatcher::getAllMatches();

    qDebug() << "BENCHMARK: RibbonMatcher::getAllMatches(): " << stopwatch.restart() << "msec"; // this apparently takes a _long_ time
    qDebug() << "fqr.size() =" << fqr.size();

    num_matches = fqr.size(); // we fetched all matches at once

    // create the vector of FragmentConfs
    fc.reserve(fqr.size());
    for (FragmentQueryResult::const_iterator it = fqr.begin(), end = fqr.end(); it != end; it++) {
      fc.push_back(*it);
    }

    // sort the FragmentConfs by sortField as either ascending or descending
    sortFCClass sfc(sortField, sortLT);
    sortFCIdxClass sfic(sfc, fc);
    std::vector<int> fcidx(fc.size());
    for (int i = 0; i < (int) fcidx.size(); i++) fcidx[i] = i;
    std::sort(fcidx.begin(), fcidx.end(), sfic);

    std::vector<FragmentConf> oldfc = fc;
    for (int i = 0; i < (int) fcidx.size(); i++) {
      assert(fcidx[i] < fc.size());
      fc[i] = oldfc[fcidx[i]];
    }

    // update the idmap
    for (size_t i = 0; i < fc.size(); i++) {
      bool isNum = true;
      int id = fc[i].getString("id", "-1").toInt(&isNum);
      // if (!isNum) continue;
      assert(isNum);
      assert(id >= 0);
      if (idmap.contains(id)) { qDebug() << "idMap contains" << id;  continue; }
      assert(!idmap.contains(id));
      idmap[id] = i;
    }
  }

  // initiliaze the state stack
  state.clear();
  state.push_back(state_t(num_thumbs));

  // load the first batch of matches from the database if necessary
  if (fc.size() < std::min(num_thumbs, num_matches))
    loadMoreMatches(10000000 * (int) ceil((num_thumbs - fc.size()) / 10000000.0));

  s().total = num_matches;
}

void thera::matchBrowser::loadMoreMatches(int n) {
  if (!useDB) return;

  qDebug() << "Fetching 10000000 matches";
  qDebug() << fc.size() << n;

  QMap<float, QString> statusMap;
  statusMap[-1.0f] = "0"; // unknown
  statusMap[ 1.0f] = "1"; // yes
  statusMap[ 0.5f] = "2"; // maybe
  statusMap[ 0.0f] = "3"; // no

  QSqlQuery query;
  query.prepare(QString("SELECT pid, tgt, src, theta, tx, ty, error, probability, uid, eval, txt, tstamp "
                        "FROM vBrowsematches WHERE IFNULL(uid, 1) = 1 ORDER BY ") + sortField + (sortLT ? " DESC" : " ASC") + " LIMIT :start, :n");
  // query.bindValue(":sortField", sortField);
  query.bindValue(":start", (int) fc.size());
  query.bindValue(":n",     n);

  qDebug() << query.lastQuery() << sortField << fc.size() << n;

  query.exec();

  qDebug() << "... Done";

  // add fragments
  for (int iter = 0; query.next(); iter++) {
    // create FragmentConf object
    FragmentConf newConf;
    newConf.mFragments[FragmentConf::TARGET] = Database::entryIndex(query.value(1).toString());
    newConf.mFragments[FragmentConf::SOURCE] = Database::entryIndex(query.value(2).toString());
    newConf.mRelev = 1.0;
    // qDebug() << query.value(0).toString() << query.value(1).toString() << query.value(2).toString();
    // qDebug() << query.value(3).toString() << query.value(4).toString() << query.value(5).toString();

    // create metadata
    RibbonMatcher::PairAlignment pa = { vec(0, 0, 0),               // contact point
                                        query.value(6).toFloat(),   // error
                                        0,                          // overlap
                                        0,   // volume
                                        0,                          // old volume
                                        XF::trans(query.value(4).toFloat(), query.value(5).toFloat(), 0) *
                                        XF::rot(query.value(3).toFloat(), vec(0, 0, 1)), // rotate by theta, translate by tx, ty
                                        // XF(),                       // placeholder for transform
                                        statusMap[query.value(9).toFloat()], // status/eval
                                        query.value(10).toString(), // comment
                                        "", "", "", 0,              // no conflict data
                                        query.value(0).toInt(),     // id
                                        0,                          // truth
                                        query.value(7).toFloat(),   // Probability,
                                        0};                         // yassine

    // qDebug() << pa.id;
    // std::cerr << pa.xf;
    // assert(0);

    // don't add a proposal that is already in the list
    if (idmap.contains(pa.id)) continue;

    // add the proposal
    idmap[pa.id] = (int) fc.size();
    RibbonMatcher::addMetaData(newConf, pa);
    fc.push_back(newConf);
  }

  qDebug() << fc.size();

  recomputeConflicts();
}


#define THUMB_WIDTH 842
#define THUMB_HEIGHT 466
#define THUMB_GUTTER 5
thera::matchBrowser::matchBrowser(QString &dbDir_, QDir &thumbDir_, QWidget *parent /* = NULL */,
                                  int rows_, int cols_, float scale_, const QString &sortField_,
                                  bool sortLT_, bool useDB_) :
  QMainWindow(parent), dbDir(dbDir_), thumbDir(thumbDir_),
  rows(rows_), cols(cols_), scale(scale_),
  useDB(useDB_), sortField(sortField_), sortLT(sortLT_), clickedThumb(0) {

  setWindowTitle(QString::fromUtf8("\u0393\u03C1\u03AF\u03C6\u03BF\u03C2 ")  + tr("Match Browser"));

  statusMenu = new QMenu(this);
  for (int i = 0; i < CONFLICT; i++) // can't explicitly set conflict status
    statusActions[i] = statusMenu->addAction(statusNames[i]);

  statusMenu->addSeparator();
  QAction *copyAction = statusMenu->addAction(tr("Copy"));
  connect(copyAction, SIGNAL(triggered()), this, SLOT(copyMatch()));

  connect(statusMenu, SIGNAL(triggered(QAction *)), this, SLOT(statusMenuTriggered(QAction *)));

  setStatusBar(new QStatusBar());

  num_thumbs = rows * cols;
  thumbs.resize(num_thumbs);
  thumb_darkened.resize(num_thumbs);

  qsa = new QScrollArea(NULL);
  qsa->setFocusPolicy(Qt::NoFocus);
  qsa->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
  // qsa->setMinimumSize(5 * THUMB_WIDTH + 4 * THUMB_GUTTER, 4 * THUMB_HEIGHT + 3 * THUMB_GUTTER);
  qw = new QFrame(NULL);
  qw->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
  qw->setMinimumSize(cols * THUMB_WIDTH  * scale + (cols - 1) * THUMB_GUTTER,
                     rows * THUMB_HEIGHT * scale + (rows - 1) * THUMB_GUTTER);
  // qw->setLayout(grid);
  qsa->setWidget(qw);
  setCentralWidget(qsa);

  qw->setObjectName("MainFrame");
  qsa->setObjectName("MainScrollArea");
  qw->setStyleSheet("QFrame#MainFrame { background-color: black }");
  // qsa->setStyleSheet("QScrollArea#MainScrollArea { background-color: black }");

  resize(cols * THUMB_WIDTH  * scale + (cols - 1) * THUMB_GUTTER + 2,
         rows * THUMB_HEIGHT * scale + (rows - 1) * THUMB_GUTTER +
         statusBar()->minimumSizeHint().height() + 2);
  fprintf(stderr, "status bar height: %d\n", statusBar()->minimumSizeHint().height());

  QSettings settings;

  qDebug() << "dbDir ==" << dbDir;
  dbDir_initialized = !dbDir.isEmpty() &&
    Database::init(dbDir, Database::FRAGMENT, true);
  fprintf(stderr, "dbDir_initialized %d\n", (int) dbDir_initialized);
  if (dbDir_initialized) settings.setValue(SETTINGS_DB_ROOT_KEY, dbDir);
  else chooseDBDir(this);

  qDebug() << "Database chosen: " << dbDir;

#if 1
  ARRIVED_HERE;
  if (useDB) {
    // open the database
    qDebug() << "Opening database.";
    QSqlDatabase sqldb = QSqlDatabase::addDatabase("QMYSQL");
    sqldb.setHostName("hoei.cis.upenn.edu");
    sqldb.setDatabaseName("vermeulen_db");
    sqldb.setUserName("tongeren");
    sqldb.setPassword("griphos");
    sqldb.open();
  }

  ARRIVED_HERE;
  RibbonMatcher::addMetaFields();

  ARRIVED_HERE;
  reloadMatches();
#else
  QTime stopwatch; // do a little benchmarking
  stopwatch.start();

  FragmentQueryResult fqr;
  if (useDB) {
    // open the database
    qDebug() << "Opening database.";
    QSqlDatabase sqldb = QSqlDatabase::addDatabase("QMYSQL");
    sqldb.setHostName("hoei.cis.upenn.edu");
    sqldb.setDatabaseName("vermeulen_db");
    sqldb.setUserName("tongeren");
    sqldb.setPassword("griphos");
    sqldb.open();

    // set up metadata fields
    RibbonMatcher::addMetaFields();

    // query all matches
    QSqlQuery query;
    qDebug() << "Executing query.";
    query.exec("SELECT pid, tgt, src, theta, tx, ty, error, Probability, volume, uid, IFNULL(eval, 0), txt, tstamp "
               "FROM vBrowsematches ORDER BY error ASC;");
    QMap<float, QString> statusMap;
    statusMap[-1.0f] = "0"; // unknown
    statusMap[ 1.0f] = "1"; // yes
    statusMap[ 0.5f] = "2"; // maybe
    statusMap[ 0.0f] = "3"; // no
    qDebug() << query.size();
    for (int iter = 0; query.next(); iter++) {
      // create FragmentConf object
      FragmentConf fc;
      fc.mFragments[FragmentConf::TARGET] = Database::entryIndex(query.value(1).toString());
      fc.mFragments[FragmentConf::SOURCE] = Database::entryIndex(query.value(2).toString());
      fc.mRelev = 1.0;
      // qDebug() << query.value(1).toString() << query.value(2).toString();

      // create metadata
      RibbonMatcher::PairAlignment pa = { vec(0, 0, 0),               // contact point
                                          query.value(6).toFloat(),   // error
                                          0,                          // overlap
                                          query.value(8).toFloat(),   // volume
                                          0,                          // old volume
                                          XF(),                       // placeholder for transform
                                          statusMap[query.value(10).toFloat()], // status/eval
                                          query.value(11).toString(), // comment
                                          "", "", "", 0,              // no conflict data
                                          query.value(0).toInt(),     // id
                                          query.value(10).toFloat(),  // truth
                                          query.value(6).toFloat(),   // Probability,
                                          0};                         // yassine
      RibbonMatcher::addMetaData(fc, pa);
      fqr << fc;
    }
  } else {
    fqr = RibbonMatcher::getAllMatches();
  }

  qDebug() << "BENCHMARK: RibbonMatcher::getAllMatches(): " << stopwatch.restart() << "msec"; // this apparently takes a _long_ time

  qDebug() << "fqr.size() =" << fqr.size();
  fc.reserve(fqr.size());
  for (FragmentQueryResult::const_iterator it = fqr.begin(), end = fqr.end(); it != end; it++) {
    fc.push_back(*it);
  }

#if 0
  qDebug() << "BENCHMARK: fc.push_back(*it) loop: " << stopwatch.elapsed() << "msec";
  std::sort(fc.begin(), fc.end(), sortFCErr);
#else
  sortFCClass sfc(sortField, sortLT);
  sortFCIdxClass sfic(sfc, fc);
  std::vector<int> fcidx(fc.size());
  for (int i = 0; i < (int) fcidx.size(); i++) fcidx[i] = i;
  std::sort(fcidx.begin(), fcidx.end(), sfic);

  std::vector<FragmentConf> oldfc = fc;
  for (int i = 0; i < (int) fcidx.size(); i++) {
    assert(fcidx[i] < fc.size());
    fc[i] = oldfc[fcidx[i]];
  }
#endif
  idmap.clear();
  for (size_t i = 0; i < fc.size(); i++) {
    bool isNum = true;
    int id = fc[i].getString("id", "-1").toInt(&isNum);
    // if (!isNum) continue;
    assert(isNum);
    assert(id >= 0);
    if (idmap.contains(id)) { qDebug() << "idMap contains" << id;  continue; }
    assert(!idmap.contains(id));
    idmap[id] = i;
  }

  qDebug() << "idmap.size() =" << idmap.size();

  state.push_back(state_t(num_thumbs));
  s().total = fc.size();
#endif

  for (int row = 0, i = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++, i++) {
      thumbs[i] = new thumbLabel(i);
      thumbs[i]->setFixedSize(THUMB_WIDTH * scale, THUMB_HEIGHT * scale);
      connect(thumbs[i], SIGNAL(clicked(int, QMouseEvent *)), this, SLOT(clicked(int, QMouseEvent *)));
      connect(thumbs[i], SIGNAL(doubleClicked(int, QMouseEvent *)),
              this, SLOT(doubleClicked(int, QMouseEvent *)));
      connect(thumbs[i], SIGNAL(updateStatus(int)),
              this, SLOT(updateThumbnailStatus(int)));
      // grid->addWidget(thumbs[i], row, col);
      // qw->addWidget(thumbs[i]);
      thumbs[i]->setParent(qw);
      thumbs[i]->move(col * (THUMB_WIDTH * scale + THUMB_GUTTER), row * (THUMB_HEIGHT * scale + THUMB_GUTTER));

      updateThumbnail(i, i < fc.size() ? i : -1);
    }
  }

  connect(this, SIGNAL(thumbnailSelected(int, int)), this, SLOT(makeThumbnailActive(int, int)));

  // fprintf(stderr, "Computing conflicts.\n");
  recomputeConflicts();
  // fprintf(stderr, "Done computing conflicts.\n");
  updateThumbnailStatuses();
  updateStatusBar();
}

void thera::matchBrowser::chooseDBDir(QWidget *parent) {
	QSettings settings;
	QString old_dbDir;

	if (!dbDir_initialized) {
		// When this is initially called upon program startup,
		// try $THERA_DB and the directory from the settings.
		dbDir_initialized = true;
		char *env = getenv("THERA_DB");
		if (env) {
			dbDir = env;
			if (Database::init(dbDir, Database::FRAGMENT, true)) {
				settings.setValue(SETTINGS_DB_ROOT_KEY, dbDir);
				return;
			}
		}

		if (settings.contains(SETTINGS_DB_ROOT_KEY)) {
			dbDir = settings.value(SETTINGS_DB_ROOT_KEY, QString()).toString();
			if (Database::init(dbDir, Database::FRAGMENT, true)) {
				settings.setValue(SETTINGS_DB_ROOT_KEY, dbDir);
				return;
			}
		}
	} else {
		// Not first time, so presumably we *had* a previous DB...
		dbDir = settings.value(SETTINGS_DB_ROOT_KEY, QString()).toString();
		old_dbDir = dbDir;
	}

	// Either we're being called explicitly, or we didn't get a value
	// the first time around.  Ask the user.
	while (1) {
		dbDir = QFileDialog::getExistingDirectory(parent,
			tr("Choose the database root directory"),
			dbDir,
			QFileDialog::ShowDirsOnly |
			QFileDialog::DontResolveSymlinks |
			QFileDialog::DontConfirmOverwrite |
			// According to docs, native on Mac ignores initial dir
			QFileDialog::DontUseNativeDialog);

		if (dbDir.isEmpty()) { // User clicked cancel
			dbDir = old_dbDir;
			if (!dbDir.isEmpty() && Database::init(dbDir, Database::FRAGMENT, true))
				break;
			// Yuck.
			exit(1);
		}

		if (Database::init(dbDir, Database::FRAGMENT, true))
			break;

		QMessageBox::warning(this, tr("Invalid database root directory"),
			tr("Could not find a valid database at")
			+ " "
			+ (dbDir.size() ? dbDir : tr("[empty]"))
			+ ". "
			+ tr("Please choose an alternative database root directory."));
	}

	settings.setValue(SETTINGS_DB_ROOT_KEY, dbDir);
}

void thera::matchBrowser::dbUpdateStatusAndComment(const FragmentConf &c) {
  const float statusMap[] = { -1.0f, 1.0f, 0.5f, 0.0f };
  
  if (useDB) {
    QSqlQuery query;
    query.prepare("REPLACE INTO tblEvaluations (pid, uid, eval, txt) VALUES (:pid, :uid, :eval, :comment);");
    query.bindValue(":pid", c.getString("id"));
    query.bindValue(":uid", 1); // placeholder until we have actual user support
    query.bindValue(":status", statusMap[c.getString("status").toInt()]);
    query.bindValue(":comment", c.getString("comment"));
    qDebug() << "Update status and comment:" << query.exec() << query.boundValues();
  }
}

void thera::matchBrowser::setStatus(int idx, int status) {
  FragmentConf &c = fc[s().tindices[idx]];
  qDebug() << FragmentRef(c.mFragments[FragmentConf::TARGET]).id() <<
    FragmentRef(c.mFragments[FragmentConf::SOURCE]).id() <<
    c.getDouble("error", -1);

  int cur_status = c.getString("status", "0").toInt();
  if (cur_status == status) return;

  int old_status = c.getString("status", "0").toInt();

  undo_list.push_back(std::vector< std::pair<int, int> >());
  std::vector< std::pair<int, int> > &u = undo_list.back();

  c.setMetaData("status", QString::number(status));
  dbUpdateStatusAndComment(c);

  // fprintf(stderr, "pushing %d %d\n", s().tindices[idx], cur_status);
  u.push_back(std::pair<int, int>(s().tindices[idx], cur_status));
#if 0
  if (status == YES && !wasYes) {
    QStringList conflicts = c.getString("conflict", QString()).split(" ");
    qDebug() << "Conflicts for" << c.getString("id") << "are" << conflicts;
    foreach (QString idstr, conflicts) {
      if (idstr.isNull()) continue;
      //qDebug() << "Conflict:" << idstr;
      bool isNum = true;
      int id = idstr.toInt(&isNum);
      if (!isNum) continue;
      assert(isNum);
      if (!idmap.contains(id)) continue;
      int i = idmap[id];

      int old_status = fc[i].getString("status", "0").toInt();
      if (old_status == UNKNOWN || old_status == MAYBE) {
        fc[i].setMetaData("status", QString::number(CONFLICT));
        u.push_back(std::pair<int, int>(i, old_status));
      }
    }
  } else if (wasYes) {
    // recompute conflicts, any match that was placed in conflict ONLY because of the match
    // that has changed from YES to something else, should no longer be conflicted
    QSet<QString> conflict_ids;
    for (size_t i = 0; i < fc.size(); i++) {
      if (fc[i].getString("status", "0").toInt() == YES) {
        const QStringList conflicts = fc[i].getString("conflict", QString()).split(" ");

        //qDebug() << "Conflicts for" << fc[i].getString("id") << "are" << conflicts;

        foreach(const QString id, conflicts) {
          if (id.isNull()) continue;
          conflict_ids.insert(id);
        }
      }
    }

    qDebug() << "Still conflicted items: " << conflict_ids;

    for (size_t i = 0; i < fc.size(); i++) {
      int old_status = fc[i].getString("status", "0").toInt();
      if (old_status == CONFLICT) {
        QString id = fc[i].getString("id").trimmed(); // have to trim because strings are returned as " XXXXX" with one or more spaces in front

        if (!conflict_ids.contains(id)) {
          fc[i].setMetaData("status", QString::number(UNKNOWN));
          u.push_back(std::pair<int, int>(i, old_status));
        }
      }
    }
  }
#else
  if (old_status == YES || old_status == MAYBE || status == YES || status == MAYBE)
    recomputeConflicts();
#endif

  updateThumbnailStatuses();
}

void thera::matchBrowser::undo() {
  if (!undo_list.size()) return;
  std::vector< std::pair<int, int> > &u = undo_list.back();
  if (!u.size()) return;
  for (size_t i = 0; i < u.size(); i++) {
    fc[u[i].first].setMetaData("status", QString::number(u[i].second));
  }
  undo_list.pop_back();
  updateThumbnailStatuses();
}

void thera::matchBrowser::clicked(int idx, QMouseEvent *event) {
  switch (event->buttons()) {
  case Qt::LeftButton: {
    int fcidx = s().tindices[idx];

    emit thumbnailSelected(idx, clickedThumb);
    clickedThumb = idx;

    if (event->modifiers() == Qt::ShiftModifier) {
      std::cerr << qPrintable(thumbName(fc[fcidx])) << std::endl;

      copyMatch();
    }
    else if (event->modifiers() == Qt::ControlModifier) {
      QString comment = fc[fcidx].getString("comment", "");
      bool ok;
      comment = QInputDialog::getText(this, tr("Comment"), tr("Comment") + ":", QLineEdit::Normal, comment, &ok);
      if (ok) {
        fc[fcidx].setMetaData("comment", comment);
        dbUpdateStatusAndComment(fc[fcidx]);
        updateThumbnail(idx, s().tindices[idx]);
      }
    }
  } break;
  case Qt::MidButton: {
    // update status to matching
    setStatus(idx, YES);
  } break;

  case Qt::RightButton: {
    if (event->modifiers() == Qt::NoModifier) {
      emit thumbnailSelected(idx, clickedThumb);
      clickedThumb = idx;

      statusMenu->popup(QCursor::pos());
    } else if (event->modifiers() == Qt::ControlModifier | Qt::AltModifier) {
#if 0
      // delete this thumbnail
      FragmentConf &c = fc[s().tindices[idx]];
      QFile f(thumbDir.absoluteFilePath(thumbName(c)));
      f.remove();
      updateThumbnail(idx, s().tindices[idx]);
#endif
    }
  } break;
  }
}

void thera::matchBrowser::copyMatch() {
  qDebug() << "Copy triggered on thumbnail" << clickedThumb;

  TabletopModel pairModel;
  int idx = clickedThumb;

  const FragmentConf &c = fc[s().tindices[idx]];

  FragmentRef target(c.mFragments[FragmentConf::TARGET]);
  FragmentRef source(c.mFragments[FragmentConf::SOURCE]);

  pairModel.fragmentPlace(target.id(), XF());
  pairModel.fragmentPlace(source.id(), c.mXF);
  pairModel.group(QSet<const Placement *>()
                  << pairModel.placedFragment(target.id())
                  << pairModel.placedFragment(source.id()));

  // add pair annotation
  QString status  = statusNames[c.getString("status", "0").toInt()];
  QString comment = c.getString("comment", QString());
  PairAnnotation pa(source.id(), target.id(), status, comment);
  pairModel.addPairAnnotation(pa);

  QString xml = writeToString(pairModel);
  QApplication::clipboard()->setText(xml);
}

void thera::matchBrowser::copyAll() {
  // construct clean TabletopModel which will contain all pieces currently in the matchbrowser
  TabletopModel model;

  // collect the indices of all fragment matches that pass the current settings (filters, ...)
  std::vector<int> valid;
  currentValidIndices(valid);

  // figure out which fragments should be grouped together
  QHash<int, int> groups;
  int next_group = 0;
  for (size_t i = 0; i < valid.size(); i++) {
    const FragmentConf &c = fc[valid[i]];

    int tgt = c.mFragments[FragmentConf::TARGET];
    int src = c.mFragments[FragmentConf::SOURCE];

    FragmentRef target(tgt);
    FragmentRef source(src);

    if (groups.contains(tgt) && groups.contains(src)) {
      // both tgt and src fragments have been seen already
      int tidx = groups[tgt], sidx = groups[src];
      if (tidx == sidx) continue; // and they're already in the same group

      qDebug() << "Computing motion based on" << FragmentRef(source) << FragmentRef(target);
      std::cerr << "c.mXF\n" << c.mXF;
      XF dxf = model.placedFragment(target)->accumXF() * c.mXF;
      std::cerr << "dxf\n" << dxf;
      dxf = dxf * inv(model.placedFragment(source)->accumXF());
      std::cerr << "dxf+\n" << dxf;

      // merge the groups containing src and tgt
      foreach (int key, groups.keys()) {
        if (groups[key] == sidx) {
          groups[key] = tidx;
          const Placement *p = model.placedFragment(FragmentRef(key));
          assert(p);
          qDebug() << "Moving" << FragmentRef(key);
          XF xf = p->accumXF();
          std::cerr << xf;
          model.setXF(p, p->xf() * inv(xf) * dxf * xf);
        }
      }
    } else if (groups.contains(tgt)) {
      // src should go in the same group as tgt
      groups[src] = groups[tgt];
      model.fragmentPlace(source, model.placedFragment(target)->accumXF() * c.mXF);
    } else if (groups.contains(src)) {
      // tgt should go in the same group as src
      groups[tgt] = groups[src];
      model.fragmentPlace(target, model.placedFragment(source)->accumXF() * inv(c.mXF));
    } else {
      // src and tgt should go in a new group
      groups[src] = next_group++;
      groups[tgt] = groups[src];

      model.fragmentPlace(target, XF());
      model.fragmentPlace(source, c.mXF);
    }
  }

  // loop over all matches (in the fc vector) and add them to the model in the appropriate group
  std::vector< QSet<const Placement *> > placedFragments(next_group);

  for (size_t i = 0; i < valid.size(); i++) {
    const FragmentConf &c = fc[valid[i]];

    const FragmentRef target(c.mFragments[FragmentConf::TARGET]);
    const FragmentRef source(c.mFragments[FragmentConf::SOURCE]);

#if 0
    //qDebug() << "matchBrowser::copyAll: Attempting to place valid[" << i << "]. Match id =" <<  c.getString("id");

#if 0
    model.fragmentPlace(target.id(), XF());
    model.fragmentPlace(source.id(), c.mXF);
#else
    const PlacedFragment *ptgt = model.placedFragment(target.id());
    const PlacedFragment *psrc = model.placedFragment(source.id());
    if (!ptgt && !psrc) {
      qDebug() << "Placing both" << target.id() << "and" << source.id();
      model.fragmentPlace(target.id(), XF());
      model.fragmentPlace(source.id(), c.mXF);
    } else if (!ptgt) {
      model.fragmentPlace(target.id(), psrc->accumXF() * inv(c.mXF));
    } else if (!psrc) {
      qDebug() << "Placing source" << source.id() << "next to" << target.id();
      model.fragmentPlace(source.id(), ptgt->accumXF() * c.mXF);
    } // else both fragments are already placed
#endif
#endif

    // add pair annotation
    QString status  = statusNames[c.getString("status", "0").toInt()];
    QString comment = c.getString("comment", QString());
    PairAnnotation pa(source.id(), target.id(), status, comment);
    model.addPairAnnotation(pa);

    // const PlacedFragment *placedTarget = model.placedFragment(target.id());
    // const PlacedFragment *placedSource = model.placedFragment(source.id());

    // qDebug() << "matchBrowser::copyAll: placedTarget was" << ((placedTarget == NULL) ? "NULL" : "OK") << ". Match id =" << c.getString("id") << ", Fragment id =" << target.id();
    // qDebug() << "matchBrowser::copyAll: placedSource was" << ((placedSource == NULL) ? "NULL" : "OK") << ". Match id =" << c.getString("id") << ", Fragment id =" << source.id();

    assert(groups.contains(c.mFragments[FragmentConf::TARGET]));
    assert(groups.contains(c.mFragments[FragmentConf::SOURCE]));
    qDebug() << c.mFragments[FragmentConf::TARGET] << groups[c.mFragments[FragmentConf::TARGET]];
    qDebug() << c.mFragments[FragmentConf::SOURCE] << groups[c.mFragments[FragmentConf::SOURCE]];
    int g = groups[c.mFragments[FragmentConf::TARGET]];
    assert(g == groups[c.mFragments[FragmentConf::SOURCE]]);
    assert(g < placedFragments.size());
    placedFragments[g]
      << model.placedFragment(target.id())
      << model.placedFragment(source.id());
  }

  for (size_t i = 0; i < placedFragments.size(); i++)
    if (placedFragments[i].size())
      model.group(placedFragments[i]);

  // write current TabletopModel to string and copy to clipboard
  QString xml = writeToString(model);
  QApplication::clipboard()->setText(xml);
}

void thera::matchBrowser::statusMenuTriggered(QAction *action) {
  qDebug() << "Triggered" << action->text() << "on thumbnail" << clickedThumb;
  int i = 0;
  for (i = 0; i < NUM_STATUSES; i++)
    if (statusActions[i] == action) break;

  if (i < NUM_STATUSES) setStatus(clickedThumb, i);
}

void thera::matchBrowser::makeThumbnailActive(int newidx, int formeridx) {
  updateThumbnailStatus(formeridx);
  updateThumbnail(formeridx, s().tindices[formeridx]);

  QLabel &newthumb = *(thumbs[newidx]);
  QPixmap p = *(newthumb.pixmap());
  QPainter painter(&p);
  QPen pen(QColor(128,128,128,128));

  pen.setWidth(5);
  painter.setPen(pen);
  painter.drawRect(newthumb.rect());
  newthumb.setPixmap(p);
}

void thera::matchBrowser::doubleClicked(int idx, QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    state.push_back(s());
    s().cur_pos = -1;
    s().conflict_index = s().tindices[idx];
    scroll(1);
  }
}

void thera::matchBrowser::updateThumbnailStatus(int i) {
  QPixmap p = *(thumbs[i]->pixmap());
  QPainter painter(&p);

  int idx = s().tindices[i];
  if (idx < 0 || idx >= fc.size()) return;
  int status = fc[idx].getString("status", "0").toInt();

  QColor c = Qt::white; // should never happen
  switch (status) {
  case UNKNOWN: c = Qt::black; break; // unknown
  case YES: c = Qt::green; break; // correct
  case MAYBE: c = QColor(255, 128, 0); break; // maybe
  case NO: c = Qt::red; break; // no
  case CONFLICT: c = /* Qt::magenta */ QColor(128, 128, 128); break; // no by conflict
  };
  painter.fillRect(0, 0, thumbs[i]->width(), 10, c);

  int conflict = checkConflict(fc[idx]);

  qDebug() << conflict;

  if (conflict > 0 && status != YES && status != MAYBE) {
    qDebug() << "Conflict!";
    painter.fillRect(thumbs[i]->width() / 2, 0, thumbs[i]->width(), 10, Qt::magenta);
    if (conflict == 2 && status != MAYBE)
      painter.fillRect(3 * thumbs[i]->width() / 4, 0, thumbs[i]->width(), 10, QColor(255, 128, 0));
  }

  if (conflict && !thumb_darkened[i] && status != YES && status != MAYBE /* && status != NO */) {
    painter.fillRect(0, 0, thumbs[i]->width(), thumbs[i]->height(), QColor(0, 0, 0, 96)); // darken conflicted thumbnails
    thumb_darkened[i] = true;
  }

  thumbs[i]->setPixmap(p);
}

void thera::matchBrowser::scroll(int amount) {
  int new_pos = std::max(s().cur_pos + amount, 0);
  if (new_pos == s().cur_pos) return;

  // fprintf(stderr, "conflict index = %d\n", s().conflict_index);
  qDebug() << "Filter =" << s().filter << "IsEmpty:" << s().filter.isEmpty();

  std::vector<int> valid;
  currentValidIndices(valid); // fill the vector with all currently valid indices (matching filter, etc.)

  // load more matches if necessary, 10000000 at a time
  while ((valid.size() < new_pos + num_thumbs) && fc.size() < num_matches) {
    loadMoreMatches(10000000);
    currentValidIndices(valid); // recompute the valid indices
  }

  // update the current state
  s().total = (int) valid.size();
  new_pos = std::max(0, std::min(new_pos, (int) valid.size() - num_thumbs));
  if (new_pos == s().cur_pos) return;
  s().cur_pos = new_pos;

  // reload thumbnails
  for (int i = 0; i < num_thumbs; i++) {
    // if (i + new_pos) doesn't fit in valid.size(), load an empty thumbnail (-1)
    updateThumbnail(i, (valid.size() > i + new_pos) ? valid[i + new_pos] : -1);
  }

  updateStatusBar();
}

void thera::matchBrowser::currentValidIndices(std::vector<int>& valid) {
  // it's possible the vector still contains data, better to make sure and clear it
  valid.clear();

  std::vector<bool> v(fc.size());

  if (s().conflict_index < 0) {
    // this means we're not looking for all conflicts to s().conflict_index, business as usual

    for (int i = 0, ii = (int) fc.size(); i < ii; i++) {
      int status = fc[i].getString("status", "0").toInt();

      if (!s().show_unknown && status == UNKNOWN) continue;
      if (!s().show_rejected && status == NO) continue;
      if (!s().show_conflicted && status == CONFLICT) continue;
      if (!s().show_maybe && status == MAYBE) continue;
      if (!s().show_yes && status == YES) continue;

      // skip conflicts with yes
      if (!s().show_conflicted && (status == UNKNOWN || status == NO)) {
        if (!conflicted_match.contains(i)) conflicted_match[i] = checkConflict(fc[i]);
        if (conflicted_match[i]) continue;
      }

      QString tid = FragmentRef(fc[i].mFragments[FragmentConf::TARGET]).id(); // target id
      QString sid = FragmentRef(fc[i].mFragments[FragmentConf::SOURCE]).id(); // source id

      QRegExp filter(s().filter, Qt::CaseSensitive, QRegExp::Wildcard);
      QString text = tid + "<>" + sid;
      if (!s().filter.isEmpty() && !text.contains(filter)) {
        // qDebug() << "Filtered" << text;
        continue;
      }

      if (!(s().show_conflicted || s().show_rejected || s().show_unknown)) {
        // fprintf(stderr, "%d %s\n", status, qPrintable(thumbName(fc[i])));
      }

      // valid.push_back(i);
      v[i] = true;
    }

    if (s().expand) {
      QSet<QString> frags;
      for (size_t i = 0; i < v.size(); i++) {
        if (v[i]) {
          frags.insert(fc[i].getTargetId());
          frags.insert(fc[i].getSourceId());
        }
      }

      for (bool changed = true; changed; ) {
        changed = false;

        for (size_t i = 0; i < fc.size(); i++) {
          if (v[i]) continue;

          if (frags.contains(fc[i].getTargetId()) ^ frags.contains(fc[i].getSourceId())) {
            int status = fc[i].getString("status", "0").toInt();

            if ((s().show_yes && status == YES) || (s().show_maybe && status == MAYBE)) {
              frags.insert(fc[i].getTargetId());
              frags.insert(fc[i].getSourceId());

              qDebug() << "Expanded to" << fc[i].getTargetId() << fc[i].getSourceId();

              changed = true;
              v[i] = true;
            }
          }
        }
      }
    }

    for (int i = 0, ii = (int) v.size(); i < ii; i++)
      if (v[i]) valid.push_back(i);
  } else {
    // in this case (s().conflict_index >= 0), load everything that conflicts with s().conflict_index, and the element itself

    QStringList conflicts;

    conflicts << fc[s().conflict_index].getString("id").trimmed(); // select the element
    conflicts << fc[s().conflict_index].getString("conflict", QString()).split(" "); // select the elements it conflicts with

    //qDebug() << "matchBrowser::currentValidIndices: conflict_index =" << s().conflict_index << ">= 0:"
    //    << "id of conflict search fragment:" << fc[s().conflict_index].getString("id")
    //    << "conflicts with:" << conflicts;

    for (int i = conflicts.size() - 1; i >= 0; i--) {
      // remove all id's that are not legal

      bool isNum;
      int id = conflicts[i].toInt(&isNum);
      if (isNum && idmap.contains(id)) {
        assert(idmap[i] < fc.size());

        valid.push_back(idmap[id]);

        //qDebug() << "matchBrowser::currentValidIndices: pushed back idmap[" << id << "] =" << idmap[id];
      }
    }
  }
}

void thera::matchBrowser::currentValidConfigurations(std::vector<FragmentConf>& valid) {
  // it's possible the vector still contains data, better to make sure and clear it
  valid.clear();

  // collect the indices of all fragment matches that pass the current settings (filters, ...)
  std::vector<int> validInd;
  currentValidIndices(validInd);

  // loop over all matches (in the fc vector) and add them to the configurations vector
  for (std::vector<int>::const_iterator it = validInd.begin(), end = validInd.end(); it != end; it++) {
    valid.push_back(fc[*it]);
  }
}

void thera::matchBrowser::updateStatusBar() {
  int last_idx = num_thumbs - 1;
  for (; last_idx >= 0 && s().tindices[last_idx] < 0; last_idx--) ;

  if (last_idx >= 0) {
    QString message = QString("Browsing %1 (%2) to %3 (%4) of %5.");
    message = message.arg(s().cur_pos + 1).arg(fc[s().tindices[0]].getString("error"));
    message = message.arg(s().cur_pos + last_idx + 1).arg(fc[s().tindices[last_idx]].getString("error"));
    message = message.arg(s().total);
    statusBar()->showMessage(message);
  } else statusBar()->showMessage("");
}

void thera::matchBrowser::keyPressEvent(QKeyEvent *event) {
  // fprintf(stderr, "Key: %d\n", event->key());
  switch(event->key()) {
  case Qt::Key_N: {
    // change all UNKNOWN's to NO

    bool changed = false;
    undo_list.push_back(std::vector< std::pair<int, int> >());
    for (int i = 0; i < num_thumbs; i++) {
      int status = fc[s().tindices[i]].getString("status", "0").toInt();
      if (status == UNKNOWN) {
        fc[s().tindices[i]].setMetaData("status", QString::number(NO));
        undo_list.back().push_back(std::pair<int, int>(s().tindices[i], status));
        changed = true;
      }
    }
    if (changed) updateThumbnailStatuses();
    else undo_list.pop_back();
  }

  case Qt::Key_Z: {
    if (event->modifiers() == Qt::ControlModifier) undo();
  } break;

  case Qt::Key_0: {
    state.push_back(s());
    s().show_rejected = false;
    s().show_conflicted = false;
    s().show_maybe = false;
    s().show_yes = false;
    s().show_unknown = true;
    s().cur_pos = -1;
    scroll(1);
  } break;

  case Qt::Key_Y: {
    state.push_back(s());
    s().show_rejected = false;
    s().show_conflicted = false;
    s().show_maybe = false;
    s().show_yes = true;
    s().show_unknown = false;
    s().cur_pos = -1;
    scroll(1);
  } break;

  case Qt::Key_U: {
    state.push_back(s());
    s().show_rejected = false;
    s().show_conflicted = false;
    s().show_maybe = false;
    s().show_yes = (event->modifiers() & Qt::ShiftModifier) ? false : true;
    s().show_unknown = true;
    s().cur_pos = -1;
    scroll(1);
  } break;

  case Qt::Key_M: {
    state.push_back(s());
    s().show_rejected = false;
    s().show_conflicted = false;
    s().show_maybe = true;
    s().show_yes = (event->modifiers() & Qt::ShiftModifier) ? false : true;
    s().show_unknown = false;
    s().cur_pos = -1;
    scroll(1);
  } break;

  case Qt::Key_F: {
    bool ok = false;
    QString filter = QInputDialog::getText(this, tr("Filter"), tr("Filter") + ":", QLineEdit::Normal, "", &ok);
    if (ok && filter != s().filter) {
      state.push_back(s());
      s().filter = filter;
      s().cur_pos = -1;
      scroll(1);
    }
  } break;

  case Qt::Key_E: {
    // expand current set to all connected yesses (if yes is shown) and maybes (if maybe is shown)
    state.push_back(s());
    s().expand = !s().expand;
    s().cur_pos = -1;
    scroll(1);
    break;
  }

  case Qt::Key_PageDown:
  case Qt::Key_Space:
  case Qt::Key_Down: {
    // move forward
    int multiplier = 1; // one screen
    if (event->modifiers() == Qt::ShiftModifier) multiplier = 10; // 10 screens
    else if (event->modifiers() == Qt::ControlModifier) multiplier = 100; // 100 screens
#if 1
    scroll(multiplier * num_thumbs);
#else
    if (s().conflict_index < 0) {
      int j = std::min(s().tindices[0] + num_thumbs, (int) fc.size() - num_thumbs - 1);
      if (j <= s().tindices[0]) break;
      for (int i = 0; i < num_thumbs; i++)
        updateThumbnail(i, i + j);
    } else {
    }
#endif
  } break;

  case Qt::Key_PageUp:
  case Qt::Key_Up: {
    // move backward
    int multiplier = 1; // one screen
    if (event->modifiers() == Qt::ShiftModifier) multiplier = 10; // 10 screens
    else if (event->modifiers() == Qt::ControlModifier) multiplier = 100; // 100 screens
#if 1
    scroll(-multiplier * num_thumbs);
#else
    if (s().conflict_index < 0) {
      if (s().tindices[0] == 0) break;
      int j = std::max(s().tindices[0] - num_thumbs, 0);
      for (int i = 0; i < num_thumbs; i++)
        updateThumbnail(i, i + j);
    }
#endif
  } break;

  case Qt::Key_Backspace:
    if (state.size() > 1) {
      state.pop_back();
      for (int i = 0; i < num_thumbs; i++)
        updateThumbnail(i, s().tindices[i]);
      updateStatusBar();
    }
    break;

  case Qt::Key_C: {
    switch (event->modifiers()) {
      case Qt::ControlModifier: {
        // if ctrl+c is pressed, we copy the entire currently valid set
        copyAll();
      } break;

      /*
      case Qt::ShiftModifier: {
        if (s().conflict_index >= 0) {
          std::vector<FragmentConf> valid;
          currentValidConfigurations(valid);

          // erase the actual conflicted match from the vector (a match can't conflict with itself)
          for (std::vector<FragmentConf>::iterator it = valid.begin(), end = valid.end(); it != end; it++) {
            if ((*it).getString("id") == fc[s().conflict_index].getString("id")) {
              valid.erase(it);
              break;
            }
          }

          mConflictView.setConfigurations(fc[s().conflict_index], valid);
          mConflictView.show();
        }
      } break;
      */

    default: { // hide all matches that currently conflict with a yes
      state.push_back(s());
      conflicted_match.clear();
      s().show_conflicted = !s().show_conflicted;
      s().cur_pos = -1;
      fprintf(stderr, "Updating...");
      scroll(1);
      fprintf(stderr, "Done\n");
    }
    }
  } break;

  case Qt::Key_W:
    RibbonMatcher::writeAlignments();
    break;
  }
}

void thera::thumbLabel::downloadThumbnail(QString url) {
  netreply = nam.get(QNetworkRequest(QUrl(url)));
  netreply->setReadBufferSize(0);
  connect(netreply, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void thera::thumbLabel::downloadFinished() {
  QPixmap t;
  t.loadFromData(netreply->readAll());
  if (!t.isNull()) {
    setPixmap(t.scaledToWidth(width(), Qt::SmoothTransformation));
    netreply->deleteLater();
    netreply = NULL;
    emit updateStatus(idx);
  }
}

void thera::matchBrowser::updateThumbnail(int tidx, int fcidx) {
  s().tindices[tidx] = fcidx;

  if (fcidx < 0 || fcidx >= fc.size()) {
    QPixmap p(THUMB_WIDTH * scale, THUMB_HEIGHT * scale);
    p.fill(Qt::black);
    thumbs[tidx]->setPixmap(p);
    thumbs[tidx]->setToolTip(QString());
  } else {
    QString thumbFile = thumbName(fc[fcidx]);
    QString thumbFullFile = thumbDir.absoluteFilePath(thumbFile);
    qDebug() << thumbFile;
    std::cerr << fc[fcidx].mXF << std::endl;

    QPixmap p;

    if (!useDB)
      p = QPixmap(thumbFullFile).scaledToWidth(THUMB_WIDTH * scale, Qt::SmoothTransformation);

    if (p.isNull()) {
      p = QPixmap(THUMB_WIDTH * scale, THUMB_HEIGHT * scale);
      p.fill(Qt::lightGray);
    }

    thumbs[tidx]->setPixmap(p);

    // retrieve thumbnail from network if necessary
    if (useDB) thumbs[tidx]->downloadThumbnail(thumbFile);

    // QString tooltip = thumbName(fc[fcidx]);
    QString tooltip = Database::fragment(fc[fcidx].mFragments[FragmentConf::TARGET])->id()
      + " -> " + Database::fragment(fc[fcidx].mFragments[FragmentConf::SOURCE])->id()
      + " | " + fc[fcidx].getString("error", "") + " | " + fc[fcidx].getString("Probability", "");
    QString comment = fc[fcidx].getString("comment", "");
    if (!comment.isEmpty()) tooltip += "\n" + comment;
    thumbs[tidx]->setToolTip(tooltip);
  }

  updateThumbnailStatus(tidx);
}

static void compute_contour_overlaps(const thera::Poly2 &src, const thera::Poly2 &tgt, const thera::XF &xf,
                                     std::vector<bool> &src_used, std::vector<bool> &tgt_used) {
  // anything marked as used will be preserved as used
  assert(src_used.size() == src.size());
  assert(tgt_used.size() == tgt.size());
  const float maxsqdist = 1;

  vec2 mn = tgt[0], mx = tgt[0];
  for (size_t j = 1; j < tgt.size(); j++) {
    mn = std::min(mn, tgt[j]);
    mx = std::max(mx, tgt[j]);
  }

  for (size_t i = 0; i < src.size(); i++) {
    // assuming planar transform
    vec2 p(xf[0] * src[i][0] + xf[4] * src[i][1] + xf[12],
           xf[1] * src[i][0] + xf[5] * src[i][1] + xf[13]);

    for (size_t j = 0; j < tgt.size(); j++) {
      if (src_used[i] && tgt_used[j]) continue;

      if (p[0] < mn[0] - maxsqdist) continue;
      if (p[0] > mx[0] + maxsqdist) continue;
      if (p[1] < mn[1] - maxsqdist) continue;
      if (p[1] > mx[1] + maxsqdist) continue;

      if (dist2(p, tgt[j]) < maxsqdist) {
        src_used[i] = 1;
        tgt_used[j] = 1;
        break;
      }
    }
  }
}

void thera::matchBrowser::recomputeConflicts() {
  yesses.clear();
  maybes.clear();
  conflicts_yes.clear();
  conflicts_maybe.clear();

  for (size_t i = 0; i < fc.size(); i++) {
    int status = fc[i].getString("status", "0").toInt();

    if (status != YES && status != MAYBE) continue;

    hash_t &hash = (status == YES) ? conflicts_yes : conflicts_maybe;

    int tgt_idx = fc[i].mFragments[FragmentConf::TARGET];
    int src_idx = fc[i].mFragments[FragmentConf::SOURCE];

    if (status == YES) yesses[tgt_idx].insert(src_idx);
    else maybes[tgt_idx].insert(src_idx);

    const Fragment *tgt = Database::fragment(tgt_idx);
    const Fragment *src = Database::fragment(src_idx);

    CPoly2 ctgt_poly = tgt->contour();
    CPoly2 csrc_poly = src->contour();
    ctgt_poly.pin();
    csrc_poly.pin();
    size_t tgt_size = (*ctgt_poly).size();
    size_t src_size = (*csrc_poly).size();

    qDebug() << "Recording conflicts with" << tgt->id() << tgt_size << "<->" << src->id() << src_size << status;

    std::vector<bool> &tgt_used = hash[tgt_idx];
    tgt_used.resize(tgt_size);

    std::vector<bool> &src_used = hash[src_idx];
    src_used.resize(src_size);

    compute_contour_overlaps(*csrc_poly, *ctgt_poly, fc[i].mXF, src_used, tgt_used);

    ctgt_poly.unpin();
    csrc_poly.unpin();

    // fprintf(stderr, "%d: %d; %d: %d\n", tgt_idx, hash.contains(tgt_idx), src_idx, hash.contains(src_idx));
  }
}

static int check_conflict(const std::vector<bool> &a, const std::vector<bool> &b) {
  assert(a.size() == b.size());

  int overlap = 0;
  for (size_t i = 0; i < a.size(); i++)
    overlap += (a[i] & b[i]);

  // fprintf(stderr, "Overlap: %d\n", overlap);
  return overlap;
}

// 0 = no conflict, 1 = conflict with yes, 2 = conflict with maybe
int thera::matchBrowser::checkConflict(const FragmentConf &conf) {
  int tgt_idx = conf.mFragments[FragmentConf::TARGET];
  int src_idx = conf.mFragments[FragmentConf::SOURCE];

  // pair already used
  if (yesses.contains(tgt_idx) && yesses[tgt_idx].contains(src_idx)) return 1;
  if (maybes.contains(tgt_idx) && maybes[tgt_idx].contains(src_idx)) return 2;

  // no examples of this pair?
  // fprintf(stderr, "%d: %d %d\n", tgt_idx, conflicts_yes.contains(tgt_idx), conflicts_maybe.contains(tgt_idx));
  // fprintf(stderr, "%d: %d %d\n", src_idx, conflicts_yes.contains(src_idx), conflicts_maybe.contains(src_idx));
  // qDebug() << conflicts_yes.contains(tgt_idx);
  // qDebug() << conflicts_yes.contains(src_idx);
  if (!(conflicts_yes.contains(tgt_idx) || conflicts_yes.contains(src_idx) ||
        conflicts_maybe.contains(tgt_idx) || conflicts_maybe.contains(src_idx)))
    return 0;

  const Fragment *tgt = Database::fragment(tgt_idx);
  const Fragment *src = Database::fragment(src_idx);

  // qDebug() << "Checking for conflict on" << tgt->id() << "<->" << src->id();

  CPoly2 ctgt_poly = tgt->contour();
  CPoly2 csrc_poly = src->contour();
  ctgt_poly.pin();
  csrc_poly.pin();
  size_t tgt_size = (*ctgt_poly).size();
  size_t src_size = (*csrc_poly).size();

  std::vector<bool> tgt_used(tgt_size);
  std::vector<bool> src_used(src_size);

  compute_contour_overlaps(*csrc_poly, *ctgt_poly, conf.mXF, src_used, tgt_used);
  // std::cerr << conf.mXF;

  ctgt_poly.unpin();
  csrc_poly.unpin();

  ARRIVED_HERE;

  // qDebug() << std::count(src_used.begin(), src_used.end(), true);
  // qDebug() << std::count(conflicts_yes[src_idx].begin(), conflicts_yes[src_idx].end(), true);
  // qDebug() << std::count(tgt_used.begin(), tgt_used.end(), true);
  // qDebug() << std::count(conflicts_yes[tgt_idx].begin(), conflicts_yes[tgt_idx].end(), true);
  // qDebug() << src->id();

  const int conflict_threshold = 10;
  if (conflicts_yes.contains(src_idx)) {
    qDebug() << check_conflict(src_used, conflicts_yes[src_idx]);
    if (check_conflict(src_used, conflicts_yes[src_idx]) > conflict_threshold)
      return 1;
  }

  if (conflicts_yes.contains(tgt_idx)) {
    qDebug() << check_conflict(tgt_used, conflicts_yes[tgt_idx]);
    if (check_conflict(tgt_used, conflicts_yes[tgt_idx]) > conflict_threshold)
      return 1;
  }

  if (conflicts_maybe.contains(src_idx))
    if (check_conflict(src_used, conflicts_maybe[src_idx]) > conflict_threshold)
      return 2;
  if (conflicts_maybe.contains(tgt_idx))
    if (check_conflict(tgt_used, conflicts_maybe[tgt_idx]) > conflict_threshold)
      return 2;

  return 0;
}
