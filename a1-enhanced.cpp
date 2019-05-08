/* the enhancement is multi player support, 2 players can playe the
 * game simultaneously.
 * Player 1 use 'w''a''s''d' to control the direction, and Player 2
 * still use the arrow keys.
 * They can start a new level only when both of them reach the goal
 * area, otherwise, the first reached one needs to wait until the
 * other one reach the goal area.
 */

#include <cstdlib>
#include <iostream>
#include <list>
#include <vector>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/time.h>

/*
 * Header files for X functions
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>

using namespace std;

// constants
const int BufferSize = 10;
int FPS =30;
int level = 1;

// constants
struct point {
    int x;
    int y;
};

//initial_position for grog_1
point initial_1 = {750/3, 200};

//initial_position for grog_2
point initial_2 = {2*750/3, 200};


struct XInfo {
    Display*    display;
    int      screen;
    Window   window;
    GC       gc;
};


class Displayable {
public:
    virtual void paint(XInfo& xinfo) = 0;
};

void error( string str ) {
  cerr << str << endl;
  exit(0);
}

class Text : public Displayable {
public:
    virtual void paint(XInfo& xinfo) {
      XDrawImageString( xinfo.display, xinfo.window, xinfo.gc,
                        this->x, this->y, this->s.c_str(), this->s.length() );
    }

    Text(int x, int y, string s): x(x), y(y), s(s)  {}

private:
    int x;
    int y;
    string s; // string to show
};

class shape: public Displayable {
public:
    virtual void paint(XInfo& xinfo) {
      XFillRectangle( xinfo.display, xinfo.window, xinfo.gc,
                      x, y, w, h);
    }

    int get_x() {
      return x;
    }

    int get_y() {
      return y;
    }

    void b_move(int rate) {
      x += rate;
    }

    string get_name() {
      return name;
    }

    void change_x(int pos) {
      x = pos;
    }
    void change_y(int pos) {
      y = pos;
    }

    void init(point initial) {
      x = initial.x;
      y = initial.y;
    }

    void move(string dir){
      int max_r = 800;
      int max_l = 0;
      int max_u = 0;
      int max_d = 200;
      if (dir == "Up"){
        if (y != max_u) y -= 50;
      } else if (dir == "Down"){
        if(y != max_u && y != max_d ) y += 50;
      } else if (dir == "Left"){
        if(x!=max_l)x -= 50;
      } else if (dir == "Right"){
        if(x != max_r) x += 50;
      } else if (dir == "n") {
        if (y == 0 ) {
          if (name == "frog2") {
            init(initial_2);
          }
          else if (name == "frog1") {
            init(initial_1);
          }
        }
      }
    }
    shape(int x, int y, int w, int h, string name):
            x(x), y(y), w(w), h(h), name(name) {}

private:
    int x;
    int y;
    int w;
    int h;
    string name;
};
/*
 * Create a window
 */
void initX(int argc, char* argv[], XInfo& xinfo) {
  XSizeHints hints;

  hints.x = 100;
  hints.y = 100;
  hints.width = 850;
  hints.height = 250;
  hints.flags = PPosition | PSize;

  /*
  * Display opening uses the DISPLAY  environment variable.
  * It can go wrong if DISPLAY isn't set, or you don't have permission.
  */
  xinfo.display = XOpenDisplay( "" );
  if ( !xinfo.display )   {
    error( "Can't open display." );
  }

  /*
  * Find out some things about the display you're using.
  */
  // DefaultScreen is as macro to get default screen index
  xinfo.screen = DefaultScreen( xinfo.display );



  unsigned long white, black;
  white = XWhitePixel( xinfo.display, xinfo.screen );
  black = XBlackPixel( xinfo.display, xinfo.screen );

  xinfo.window = XCreateSimpleWindow(
          xinfo.display,              // display where window appears
          DefaultRootWindow( xinfo.display ), // window's parent in window tree
          hints.x, hints.y,           // upper left corner location
          hints.width, hints.height,  // size of the window
          10,                         // width of window's border
          black,                      // window border colour
          white );                        // window background colour

  // extra window properties like a window title
  XSetStandardProperties(
          xinfo.display,      // display containing the window
          xinfo.window,       // window whose properties are set
          "Frog",    // window's title
          "OW",               // icon's title
          None,               // pixmap for the icon
          argv, argc,         // applications command line args
          &hints);            // size hints for the window

  /*
   * Put the window on the screen.
   */

  XSelectInput( xinfo.display, xinfo.window,
                ButtonPressMask | KeyPressMask | ButtonMotionMask );

  XMapRaised( xinfo.display, xinfo.window );

  XFlush(xinfo.display);

  sleep(1);
}

int rate;
int r_rate;

// a function for blocks moving
void block_move(vector<shape*>& blocks) {
  int line1 = -1;
  int line2 = -1;
  int line3 = -1;
  rate = level;
  r_rate = 0 - level;
  for (int i = 0; i < blocks.size(); ++i) {
    line1 = blocks[i]->get_name().find("b_1");
    line2 = blocks[i]->get_name().find("b_2");
    line3 = blocks[i]->get_name().find("b_3");
    if (line1 != -1) {
      if (blocks[i]->get_x() >= 850 + 850/3 - 50) {
        blocks[i]->change_x(0 - 50);
      }
      blocks[i]->b_move(rate);
    } else if (line2 != -1) {
      if (blocks[i]->get_x() <= 0 - 850/4) {
        blocks[i]->change_x(850);
      }
      blocks[i]->b_move(r_rate);
    } else if (line3 != -1) {
      if (blocks[i]->get_x() >= 850 + 850 /2 - 100) {
        blocks[i]->change_x(0 - 100);
      }
      blocks[i]->b_move(rate);
    }
  }
}

unsigned long now() {
  timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

void repaint( list<Displayable*> dList, XInfo& xinfo) {
  list<Displayable*>::const_iterator begin = dList.begin();
  list<Displayable*>::const_iterator end = dList.end();

  XClearWindow( xinfo.display, xinfo.window );
  while ( begin != end ) {
    Displayable* d = *begin;
    d->paint(xinfo);
    begin++;
  }
  XFlush( xinfo.display );
}


/*check whether there is a collision between blocks and frog
 * if there is, level backs to 1, and frog backs to the
 * initial position
*/
void attack(shape* frog1, shape* frog2, vector<shape*> blocks, point initi1, point initi2) {
  int line1 = -1;
  int line2 = -1;
  int line3 = -1;
  for (int i = 0; i < blocks.size(); ++i) {
    line1 = blocks[i]->get_name().find("b_1");
    line2 = blocks[i]->get_name().find("b_2");
    line3 = blocks[i]->get_name().find("b_3");
    if (frog1->get_y() == 50 && line1 != -1) {
      if ((blocks[i]->get_x()-50 < frog1->get_x()) &&
          (blocks[i]->get_x() + 50 >= frog1->get_x())) {
        frog1->init(initi1);
        frog2->init(initi2);
        level = 1;
        return;
      }
    } else if (frog1->get_y() == 100 && line2 != -1) {
      if ((blocks[i]->get_x() - 50 < frog1->get_x()) &&
          (blocks[i]->get_x() + 20 >= frog1->get_x())) {
        frog1->init(initi1);
        frog2->init(initi2);
        level = 1;
        return;
      }
    }else if (frog1->get_y() == 150 && line3 != -1) {
      if ((blocks[i]->get_x() - 50 < frog1->get_x()) &&
          (blocks[i]->get_x() + 100 >= frog1->get_x())) {
        frog1->init(initi1);
        frog2->init(initi2);
        level = 1;
        return;
      }
    }
  }
}

void eventloop(XInfo& xinfo) {
  XEvent event;
  KeySym key;
  char text[BufferSize];
  list<Displayable*> dList;
  shape* frog2 = new shape(initial_1.x, initial_1.y, 50, 50, "frog1");
  dList.push_back(frog2);
  shape* frog1 = new shape(initial_2.x, initial_2.y, 50, 50, "frog2");
  dList.push_back(frog1);
  vector<shape*> blocks;

    //initial the blocks

  shape* block_1_1 = new shape(0, 50, 50, 50, "b_1_1");
  shape* block_1_2 = new shape(850/3, 50, 50, 50, "b_1_2");
  shape* block_1_3 = new shape(2*850/3, 50, 50, 50, "b_1_3");
  shape* block_1_4 = new shape(3*850/3, 50, 50, 50, "b_1_4");
  dList.push_back(block_1_1);
  dList.push_back(block_1_2);
  dList.push_back(block_1_3);
  dList.push_back(block_1_4);
  blocks.push_back(block_1_1);
  blocks.push_back(block_1_2);
  blocks.push_back(block_1_3);
  blocks.push_back(block_1_4);

  shape* block_2_1 = new shape(0, 100, 20, 50, "b_2_1");
  shape* block_2_2 = new shape(850/4, 100, 20, 50, "b_2_2");
  shape* block_2_3 = new shape(2*850/4, 100, 20, 50, "b_2_3");
  shape* block_2_4 = new shape(3*850/4, 100, 20, 50, "b_2_4");
  shape* block_2_5 = new shape(4*850/4, 100, 20, 50, "b_2_4");
  dList.push_back(block_2_1);
  dList.push_back(block_2_2);
  dList.push_back(block_2_3);
  dList.push_back(block_2_4);
  dList.push_back(block_2_5);
  blocks.push_back(block_2_1);
  blocks.push_back(block_2_2);
  blocks.push_back(block_2_3);
  blocks.push_back(block_2_4);
  blocks.push_back(block_2_5);

  shape* block_3_1 = new shape(0, 150, 100, 50, "b_3_1");
  shape* block_3_2 = new shape(850/2, 150, 100, 50, "b_3_2");
  shape* block_3_3 = new shape(2*850/2, 150, 100, 50, "b_3_2");
  dList.push_back(block_3_1);
  dList.push_back(block_3_2);
  dList.push_back(block_3_3);
  blocks.push_back(block_3_1);
  blocks.push_back(block_3_2);
  blocks.push_back(block_3_3);

  unsigned long lastRepaint = 0;
  while(true) {

    //change the level
    string N = "level: ";
    ostringstream result;
    result << level;
    N = N + result.str();
    Text *levelN = new Text(770, 20, N);
    dList.push_back(levelN);

    if (XPending(xinfo.display) > 0) {
      repaint(dList, xinfo);
      XNextEvent(xinfo.display, &event);
      switch (event.type) {
        case KeyPress:
          int i = XLookupString(
                  (XKeyEvent * ) & event, text, BufferSize, &key, 0);
              if (i == 1 && text[0] == 'q') {
                XCloseDisplay(xinfo.display);
                return;
              }
              else if (i == 1 && text[0] == 'n') {
                if (frog1->get_y() == 0 && frog2->get_y() == 0) {
                  frog1->move("n");
                  frog2->move("n");
                  level += 1;
                }
                break;
              } else if (i == 1 && text[0] == 'w') {
                frog2->move("Up");
                break;
              } else if (i == 1 && text[0] == 'a') {
                frog2->move("Left");
                break;
              } else if (i == 1 && text[0] == 's') {
                frog2->move("Down");
                break;
              } else if (i == 1 && text[0] == 'd') {
                frog2->move("Right");
                break;
              }
              switch (key) {
                case XK_Up:
                  frog1->move("Up");
                      break;
                case XK_Down:
                  frog1->move("Down");
                      break;
                case XK_Left:
                  frog1->move("Left");
                      break;
                case XK_Right:
                  frog1->move("Right");
                      break;
              }
              attack(frog2, frog1, blocks, initial_1, initial_2);
              attack(frog1, frog2, blocks, initial_2, initial_1);
              break;
      }
    }

    block_move(blocks);
    attack(frog1, frog2, blocks, initial_2,initial_1);
    attack(frog2, frog1, blocks, initial_1,initial_2);

    unsigned long end = now();


    if (end - lastRepaint > 1000000 / FPS) {
        //set the default frame rate  for window is 30FPS
      repaint(dList, xinfo);XFlush(xinfo.display);
      lastRepaint = now();

    }

    if (XPending(xinfo.display) == 0) {

      usleep(1000000 / FPS - (end - lastRepaint));

    }

  }

}

int main ( int argc, char* argv[] ) {

  if (argc > 1){
    stringstream fps(argv[1]);
    fps >> FPS;
  }

  XInfo xinfo;

  initX(argc, argv, xinfo);

  // wait for user input to quit (a concole event for now)


  // Create 3 Graphics Contexts
  GC gc = XCreateGC(xinfo.display, xinfo.window, 0, 0);
  xinfo.gc = gc;
  XSetForeground(xinfo.display, gc, BlackPixel(xinfo.display, xinfo.screen));
  XSetBackground(xinfo.display, gc, WhitePixel(xinfo.display, xinfo.screen));
  XSetFillStyle(xinfo.display, gc, FillSolid);
  XSetLineAttributes(xinfo.display, gc,
                     1, LineSolid, CapButt, JoinRound);

  XFlush(xinfo.display);
  eventloop(xinfo);
}
