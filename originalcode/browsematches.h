/* -*- C++ -*- */

#ifndef BROWSEMATCHES_H
#define BROWSEMATCHES_H

#include <vector>

#include <QtGui>
#include <QScrollArea>
#include <QLabel>
#include <QFrame>
#include <QMainWindow>

#include <QSqlDatabase>
#include <QSqlQuery>

#include <QNetworkReply>

#include "FragmentQueryResult.h"
//#include "ConflictView.h"

namespace thera {
  class thumbLabel : public QLabel {
    Q_OBJECT;

  public:
    thumbLabel(int i, QWidget *parent = NULL) : QLabel(parent), idx(i) {}
    void downloadThumbnail(QString url);

  signals:
    void clicked(int i, QMouseEvent *event);
    void doubleClicked(int i, QMouseEvent *event);
    void updateStatus(int i);
    
  private slots:
    void downloadFinished();
    
  protected:
    virtual void mousePressEvent(QMouseEvent *event) { emit clicked(idx, event); }
    virtual void mouseDoubleClickEvent(QMouseEvent *event) { emit doubleClicked(idx, event); }

  private:
    int idx;
    QNetworkReply *netreply;
    static QNetworkAccessManager nam;
  };

  class matchBrowser : public QMainWindow {
    Q_OBJECT;

  public:
    matchBrowser(QString &dbDir_, QDir &thumbDir_, QWidget *parent,
                 int rows_, int cols_, float scale_,
                 const QString &sortField_, bool sortLT_, bool useDB_);
    void chooseDBDir(QWidget *parent = NULL);

    enum { UNKNOWN, YES, MAYBE, NO, CONFLICT, NUM_STATUSES };
    static const char *statusNames[];

  signals:
    void thumbnailSelected(int newidx, int formeridx);

  public slots:
    void clicked(int idx, QMouseEvent *event);
    void doubleClicked(int idx, QMouseEvent *event);
    void statusMenuTriggered(QAction *action);
    void copyMatch();
    void makeThumbnailActive(int newidx, int formeridx);

  protected:
    virtual void keyPressEvent(QKeyEvent *event);

  private slots:
    void updateThumbnailStatus(int i);

  private:
    void dbUpdateStatusAndComment(const FragmentConf &c);
    void setStatus(int idx, int status);
    void undo();
    void updateThumbnailStatuses() { for (int i = 0; i < num_thumbs; i++) updateThumbnailStatus(i); }
    void updateThumbnail(int tidx, int fcidx);
    void scroll(int amount);
    void currentValidIndices(std::vector<int>& valid);
    void currentValidConfigurations(std::vector<FragmentConf>& valid);
    void updateStatusBar();
    void copyAll();

    void recomputeConflicts();
    int checkConflict(const FragmentConf &fc);

    bool dbDir_initialized;
    QString dbDir;
    QDir thumbDir;
    QHash<int, int> idmap;
    std::vector<FragmentConf> fc;
    std::vector< std::vector< std::pair<int, int> > > undo_list;
    int rows, cols, num_thumbs;
    float scale;

    bool    useDB;
    QString sortField;
    bool    sortLT;
    int     num_matches;

    void loadMoreMatches(int n);
    void reloadMatches();

    // ConflictView  mConflictView;

    QScrollArea *qsa;
    QFrame *qw;
    std::vector<thumbLabel *> thumbs;
    std::vector<bool>  thumb_darkened;

    QMenu *statusMenu;
    QAction *statusActions[NUM_STATUSES];
    int clickedThumb;

    typedef QHash< int, std::vector<bool> > hash_t;
    hash_t conflicts_yes, conflicts_maybe;
    QHash< int, QSet<int> > yesses, maybes;
    QHash<int, bool> conflicted_match;

    struct state_t {
      // either src or tgt should contain filter
      // to be shown
      QString filter;

      // set to -1 for browsing the list, and
      // to a specific index to show conflicting fragments
      int conflict_index;

      // decide whether or not to show explicitly
      // rejected pairs, and those that conflict
      // with approved pairs
      bool show_rejected, show_conflicted, show_maybe, show_yes, show_unknown, expand;

      int cur_pos, total; // the current position in the list of proposals

      std::vector<int> tindices;

      state_t(int nt) : filter(""), conflict_index(-1), show_rejected(true),
                        show_conflicted(true), show_maybe(true),  show_yes(true),
                        show_unknown(true), expand(false), cur_pos(0) {
        tindices.resize(nt);
      }
    };

    std::vector<state_t> state;
    state_t &s() {
      return state.back();
    };
  };
}

#endif /* BROWSEMATCHES_H */
