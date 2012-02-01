#include <iostream>
#include <string>
#include <vector>
#include <iterator>
#include <cstdlib>
#include <cstdarg>
#include <stdexcept>
#include <cmath>
#include <ctime>

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>

#include <unistd.h>
#include <err.h>

using namespace std;

#define NELEMS(a) (sizeof(a)/sizeof((a)[0]))
#define END_OF(a) (a + NELEMS(a))

#define PI 3.14
#define E 0.0001

#define SCREEN_W 640
#define SCREEN_H 480
#define SCREEN_B 24
#define SCREEN_F SDL_SWSURFACE|SDL_DOUBLEBUF

SDL_Surface *screen;

struct Point;
typedef vector<Point> PointVector;

void rotate(Point &p, double angle);
void rotoMoveVertices(PointVector &src, PointVector &dst, Point &v, Point &c, double angle);
int draw_line(SDL_Surface *, Point &a, Point &b, Uint32 color = 0);
double distance(Point &a, Point &b, Point &c);
double point_distance(Point &a, Point &b);

struct Point {
    double x;
    double y;

    Point() : x(0.0), y(0.0) {}
    Point(double x, double y) : x(x), y(y) {}
    double length(void) const { 
        return sqrt(x*x + y*y);
    }
    Point &invert(void) {
        x = -x;
        y = -y;
        return *this;
    }
    Point &operator+= (const Point &p) {
        x += p.x;
        y += p.y;
        return *this;
    }
    Point &operator-= (const Point &p) {
        x -= p.x;
        y -= p.y;
        return *this;
    }
};
ostream &
operator<< (ostream &os, const Point &p) {
    return os << "{ " << p.x << " " << p.y << " }";
} 
bool
operator== (const Point &p1, const Point &p2) {
    return p1.x == p2.x && p1.y == p2.y;
}
Point
operator+ (const Point &p1, const Point &p2) {
    Point p(p1);
    return p += p2;
}
Point
operator- (const Point &p1, const Point &p2) {
    Point p(p1);
    return p -= p2;
}
Point
operator* (const Point &p1, const double d) {
    Point p(p1);
    p.x *= d;
    p.y *= d;
    return p;
}
Point
operator/ (const Point &p1, const double d) {
    Point p(p1);
    p.x /= d;
    p.y /= d;
    return p;
}
/*
inline Point
operator* (const double d, Point &p) { 
    return operator*(p, d);
}
*/
struct Polygon {
    PointVector ov; // original vertices
    PointVector cv; // current vertices (after rotomove)
    PointVector nextv;
    double angle;
    double nextangle;
    Point c;
    Point pos;
    Point nextpos;
    double _area;

    Point V;
    double W; // rotation speed (angle)

    Point C;
    double M;
    double I;
    double R;

    Polygon() : angle(0.0), nextangle(0.0), _area(-1.0), W(0.0), M(0.0), I(0.0), R(0.0) {}

    Polygon &rotoMove(Point v, double angle = 0.0) {
        nextangle = angle;
        nextpos = v;
        rotoMoveVertices(ov, nextv, v, c, angle);
        return *this;
    }
    Polygon &move(double dt) {
        double d, w;
        Point v;

        v = V * dt;
        w = W * dt;

        nextangle = angle + w;
        d = nextangle < 0 ? 360.0 : -360.0;
        while (fabs(nextangle) > 360.0)
            nextangle += d;
        nextpos = pos + v;
        rotoMoveVertices(ov, nextv, nextpos, C, angle);
        return *this;
    }
    Polygon &commit(void) {
        cv = nextv;
        angle = nextangle;
        pos = nextpos;
        return *this;
    } 
    Polygon &addVertex(Point &v) {
        _area = -1.0;
        ov.push_back(v);
        rotoMove(pos, angle);
        commit();
        return *this;
    }
    Polygon &draw(SDL_Surface *surf, Uint32 border_color = 0) {
        int i, next;
        Uint32 color, red[3];

        /*
        for (i = 0; i < NELEMS(red); ++i)
            red[i] = SDL_MapRGB(surf->format, 0xff, (0xff / NELEMS(red)) * i, (0xff / NELEMS(red)) * i);
        */
        
        for (i = 0; i < cv.size(); ++i) {
            next = (i + 1) % cv.size();
            /*
            if (i < NELEMS(red))
                color = red[i];
            else
                color = border_color;
            */
            color = border_color;
            draw_line(surf, cv[i], cv[next], color);
        }
        return *this;
    }
    double area(void) {
        PointVector::size_type i, j, n;

        if (_area != -1.0)
            return _area;

        _area = 0.0; 
        n = ov.size();
        for (i = 0; i < n; ++i) {
            j = (i + 1) % n;
            _area += ov[i].x * ov[j].y - ov[i].y * ov[j].x;
        }
        _area /= 2.0;

        return _area;
    }
    Point centerOfMass(void) {
        PointVector::size_type i, j, n;
        double tmp, d;
        Point centroid;

        for (i = 0, n = ov.size(); i < n; ++i) {
            j = (i + 1) % n;
            tmp = ov[i].x * ov[j].y - ov[j].x * ov[i].y;
            centroid.x += (ov[i].x + ov[j].x) * tmp;
            centroid.y += (ov[i].y + ov[j].y) * tmp;
        }
        d = 6 * area();
        centroid.x /= d;
        centroid.y /= d;

        //clog << "center_of_mass: " << centroid << endl;

        return centroid;
    } 
    double outerRadius(void) {
        Point p;
        PointVector::size_type i;
        double d, l = 0.0;
        
        for (i = 0; i < ov.size(); ++i) {
            if ((d = point_distance(ov[i], C)) > l)
                l = d;
        }
        return l;
    } 
    Polygon &updateConstants(void) {
        C = centerOfMass();
        R = outerRadius();
        M = R * R;
        I = 0.5 * M * R * R;
    }
};
ostream &
operator<< (ostream &os, const Polygon &poly) {
    ostream_iterator<Point> oit(os, " ");

    os << "{ ";
        os << "{ " << poly.V << " " << poly.W << " } ";

        os << "{ ";
        copy(poly.cv.begin(), poly.cv.end(), oit);
        os << " }";
    os << " }";
    return os;
}

int
draw_line(SDL_Surface *surf, Point &a, Point &b, Uint32 color) {
    return lineColor(surf, (Sint16) a.x, (Sint16) a.y, (Sint16) b.x, (Sint16) b.y, color);
}

string
strprintf(const char *fmt, ...) {
    va_list ap;
    char buf[8192];
    const size_t bufsize = sizeof(buf);

    va_start(ap, fmt);
    vsnprintf(buf, bufsize, fmt, ap);
    va_end(ap);

    return buf;
}

void
rotate(Point &p, double angle) {
    double xx, sin_a, cos_a;

    sin_a = sin(angle);
    cos_a = cos(angle);

    xx = p.x * cos_a - p.y * sin_a;
    p.y = p.y * cos_a + p.x * sin_a;
    p.x = xx;
}

void
rotate(Point &p, double angle, Point &c) {
    p -= c;
    rotate(p, angle);
    p += c;
}

void
rotoMoveVertices(PointVector &src, PointVector &dst, Point &v, Point &c, double angle) {
    PointVector::iterator it;

    dst.assign(src.begin(), src.end());

    for (it = dst.begin(); it != dst.end(); ++it) {
        rotate(*it, angle, c);
        *it += v;
    }
}

bool
line_equation(Point a, Point b, Point &eq) {
    double D, da, db;
    
    if (a.x == b.x)
        a.x = b.x + 0.00001;

    D = a.x - b.x;
    da = a.y - b.y;
    db = a.x * b.y - a.y * b.x;

    eq.x = da / D;
    eq.y = db / D;

    return true;
}

bool
line_equation_intersect(Point &eq1, Point &eq2, Point *p) {
    double D, dy, dx;
    Point isec; // intersection point

    D = -eq2.x - (-eq1.x);
    if (D == 0)
        return false;
    dy = eq1.y * (-eq2.x) - (-eq1.x) * eq2.y;
    dx = eq2.y - eq1.y;

    isec.x = dx / D;
    isec.y = dy / D;

    if (p)
        *p = isec;

    return true;
}

bool
does_belong_to_line(Point a, Point b, Point c) {
    b -= a;
    c -= a;
    return fabs(c.x) <= fabs(b.x);
} 
bool
line_intersect(Point &a1, Point &a2, Point &b1, Point &b2, Point &X) {
    Point eq_a, eq_b;
    bool retval;

    line_equation(a1, a2, eq_a);
    line_equation(b1, b2, eq_b);
    line_equation_intersect(eq_a, eq_b, &X);
    retval = does_belong_to_line(a1, a2, X) && does_belong_to_line(b1, b2, X);
    return retval;
}

double
point_distance(Point &a, Point &b) {
    Point c(b);
    c -= a;
    return fabs(c.length());
}

double
distance(Point &a, Point &b, Point &c) {
    double dx, dy, D;

    dx = a.x - b.x;
    dy = a.y - b.y;
    D = dx * (c.y - a.y) - dy * (c.x - a.x);

    return fabs(D / sqrt(dx * dx + dy * dy));
}


/*
struct DOT2D {
    double x, y;
    DOT2D *next;

    DOT2D() : x(0.0), y(0.0), next(NULL) {}
    DOT2D(double x, double y, DOT2D *next = NULL) : x(x), y(y), next(next) {} 

    DOT2D operator+ (DOT2D d) {
        return DOT2D(x + d.x, y + d.y);
    }
    DOT2D operator- (DOT2D d) {
        return DOT2D(x - d.x, y - d.y);
    }
    DOT2D &operator+= (DOT2D d) {
        x += d.x;
        y += d.y;
        return *this;
    }
    DOT2D &operator-= (DOT2D d) {
        x -= d.x;
        y -= d.y;
        return *this;
    }
};

struct POLY2D {
    DOT2D *first;
    size_t n;

    double square(void) {
        size_t i;
        double S = 0.0;

        if (n < 0)
            return S;

        DOT2D *p, *next;

        for (i = 0; i < n; ++i) {
            if ((next = p->next) == NULL)
                next = first;
            S += p->x * next->y - next->x * p->y;
            p = next;
        }
        return fabs(S) * 0.5;
    }
}; 
*/

Point
angle_vector(double angle) {
    return Point(cos(angle), sin(angle));
}

double
angle_of(const Point &p) {
    double a = acos(p.x);

    if (p.y < 0)
        a = (PI + PI) - a;
    return a;
} 

Point
line_normal(const Point &a, const Point &b) {
    Point v(b - a);
    return angle_vector(angle_of(v) - PI/2);
}

double
orient(Point &a, Point &b, Point &c) {
    return (a.x - c.x) * (b.y - c.y) - (a.y - c.y) * (b.x - c.x);
}

void
init_sdl(void) {
    if (SDL_Init(SDL_INIT_VIDEO) == -1)
        err(1, "SDL_Init: %s", SDL_GetError());
    atexit(SDL_Quit);
    if ((screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, SCREEN_B, SCREEN_F)) == NULL)
        err(1, "SDL_SetVideoMode: %s", SDL_GetError());
}

Uint32 ticks;

struct timediff {
    Uint32 ticks;
    Uint32 diff;
} timediff;

typedef vector<Polygon> PolygonVector;

bool
compar_vertices(Point &a, Point &b) {
}

int
rrand(int min, int max) {
    int n;

    return (rand()%(abs(max-min))) + min;
}

double
frrand(double min, double max) {
    double range = max - min;

    return ((double) rand() / (double) RAND_MAX) * range + min;
}

Point
gen_point(int xmin, int xmax, int ymin, int ymax) {
    Point p;

    p.x = (double) rrand(xmin, xmax);
    p.y = (double) rrand(ymin, ymax);

    return p;
}


void
gen_polys(PolygonVector &polys, int n, int nvertices_min, int nvertices_max) {
    int i, j, k, nvertices, niter;
    const int max_niter = 40;
    bool is_convex;

    nvertices = rrand(nvertices_min, nvertices_max); 
    polys.clear();

    for (i = 0; i < n; /* empty */) { 
        Polygon poly;
        Point p;

        for (j = 0; j < nvertices; ++j) { 
            niter = 0;

            do {
                ++niter;
                p = gen_point(-80, 80, -80, 80);
                if (j < 1)
                    is_convex = true;
                else {
                    for (k = 1; k <= j; ++k) {
                        is_convex = orient(poly.ov[k-1], poly.ov[k], p) < 0.0;
                        if (!is_convex)
                            break;
                    }

                    if (!is_convex)
                        continue;

                    for (k = 2; k <= j; ++k) {
                        is_convex = orient(poly.ov[0], poly.ov[k], p) < 0.0;
                        if (!is_convex)
                            break;
                    }

                    if (!is_convex)
                        continue;

                    is_convex = orient(poly.ov[j], poly.ov[0], p) > 0.0;
                }
            } while (!is_convex && niter < max_niter);

            if (niter >= max_niter)
                break; 

            poly.addVertex(p);
        } 
        if (niter >= max_niter)
            continue;
        //poly.c = poly.centerOfMass();
        poly.V = Point(frrand(-0.01, 0.01), frrand(-0.01, 0.01));
        poly.W = frrand(-0.0001, 0.0001);
        poly.rotoMove(Point(rrand(100, SCREEN_W-100), rrand(100, SCREEN_H-100))).commit();
        poly.updateConstants();
        polys.push_back(poly);
        ++i;
    }
}

bool
collide_polys(Polygon &a, Polygon &b, double td) {
    return false;
}

/* FIXME */
bool
collide_poly_with_walls(Polygon &poly, double td) {
    struct Line {
        Point a;
        Point b;
        Line() {}
        Line(Point &a, Point &b) : a(a), b(b) {}
    };
    Point wv[] = { // wall vertices
        Point(0.0, 0.0),
        Point(SCREEN_W, 0.0),
        Point(SCREEN_W, SCREEN_H),
        Point(0.0, SCREEN_H)
    };
    int i;
    PointVector::size_type j;
    bool did_collide = false;

    for (i = 0; i < NELEMS(wv); ++i) {
        for (j = 0; j < poly.nextv.size(); ++j) { 
            Point X;
            Line wall(wv[i], wv[(i+1)%NELEMS(wv)]);
        
            if (!line_intersect(wall.a, wall.b, poly.nextv[j], poly.cv[j], X))
                continue;

            did_collide = true;

            /*
            Point coll_normal(line_normal(wall.a, wall.b));
            Point shoulder(X.x - poly.nextv[j].x, X.y - poly.nextv[j].y);
            double impulse = 
            */
            ///
            /*
            Fs = point_distance(poly.nextv[j], poly.cv[j]) * td * poly.M;
            arm_len = distance(poly.nextv[j], poly.cv[j], poly.nextpos);
            W = orient(poly.nextv[j], poly.cv[j], poly.nextpos) < 0.0 ? -arm_len * (Fs / poly.I) : arm_len * (Fs / poly.I);
            poly.W += W;
            */
        } 
    }

    return did_collide;
}

void
move_and_collide(double td, PolygonVector &polys, const Point walls[][2], int nwalls) {
    int i;
    PolygonVector::iterator it, jt;
    Point xp;
    Point v;

    for (i = 0; i < nwalls; ++i) {
        for (it = polys.begin(); it != polys.end(); ++it) {
            it->move(td);
        }
        for (it = polys.begin(); it != polys.end(); ++it) {
            for (jt = polys.begin(); jt != polys.end(); ++jt) {
                if (jt == it)
                    continue;
                collide_polys(*it, *jt, td);
            }
        }
        for (it = polys.begin(); it != polys.end(); ++it) {
            collide_poly_with_walls(*it, td);
        }
        for (it = polys.begin(); it != polys.end(); ++it) {
            it->commit();
        }
    }
}

int
phys(int npolys) {
    SDL_Event event;
    PolygonVector polys;
    Uint32 black = 0, white;
    double angle = 0.0;
    PolygonVector::iterator pit;
    Uint32 ticks, fps_timer = 0, diff;
    Uint32 fps = 0;
    bool pause = false;
    const Point walls[][2] = {
#       define INS_WALL(a,b,c,d) { Point(a,b), Point(c,d) }
        INS_WALL(0.0, 0.0, SCREEN_W, 0.0),
        INS_WALL(SCREEN_W, 0.0, SCREEN_W, SCREEN_H),
        INS_WALL(SCREEN_W, SCREEN_H, 0.0, SCREEN_H),
        INS_WALL(0.0, SCREEN_H, 0.0, 0.0)
    };
    const int nwalls = NELEMS(walls);

    if (npolys < 1)
        return -1;

    srand(time(NULL));

    timediff.ticks = SDL_GetTicks();
    timediff.diff = 0; 
    fps_timer = timediff.ticks + 5000;

    gen_polys(polys, npolys, 3, 7);

    for (pit = polys.begin(); pit != polys.end(); ++pit) {
        cout << *pit << endl;
    } 

    white = SDL_MapRGB(screen->format, 255, 255, 255);

    for ( ;; ) {
        timediff.ticks += timediff.diff;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                return 0;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_q:
                case SDLK_ESCAPE:
                    return 0;
                case SDLK_p:
                case SDLK_PAUSE:
                    pause = !pause;
                }
                break;
            }
        }

        if (!pause) {
            ticks = SDL_GetTicks(); 
            timediff.diff = ticks - timediff.ticks;
        
            /*
            angle += 0.001 * timediff.diff;
            while(angle > 360.0)
                angle -= 360.0; 
            */
            for (pit = polys.begin(); pit != polys.end(); ++pit) {
                //pit->rotoMove(pit->pos, angle).commit();
                //pit->move(timediff.diff);
                move_and_collide(timediff.diff, polys, walls, nwalls);
            } 
        
            ++fps;
            if (ticks > fps_timer) {
                double rate = ((double) fps / 5000.0) * 1000;
                cout << "fps = " << rate << endl;
                fps_timer = ticks + (ticks - fps_timer) + 5000;
                fps = 0;
            }
        }

        for (pit = polys.begin(); pit != polys.end(); ++pit) {
            pit->draw(screen, white);
        }
        SDL_Flip(screen);
        SDL_FillRect(screen, NULL, 0); 
        //SDL_Delay(60); 
        //SDL_Delay(40);

    }
}

int
main(int argc, char **argv) {
    int n = 4;

    if (argc > 1)
        n = atoi(argv[1]);

    init_sdl();
    phys(n);

    return 0;
}
