#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <map>
#include <unordered_map>
#include <queue>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif




//--------------------------------------------GLOBAL VARIABLES--------------
const int resolution = 100;                     //The room's resolution.  controls block size and how many blocks can fit
//in the room.  I usually keep it around 100 (making it a 100x100x100 grid)
//I have it set to handle anything up to 1,000 though it will slow down
//quickly with a high resolution since you generally will be using more voxels
//and because it would take forever to clear the arrays then.

//A resolution of 100 means that each voxel is 900 / 100 = 9 centimeters across
//if you were playing this in the dive since I think the dive is 9 meters across.


int blockPlaceType = 0;                         //normal -- black pulsing -- white pulsing -- flicker

float proximity = 5;                            //how close the brushtip is to us (or the wand)
float diameter = 3.7;                           //brush size
float sprayDiameter = 3;                        //spray radius
GLboolean draw = false;                         //are we drawing?
GLboolean erase = false;                        //are we erasing?
GLboolean fullMotion = true;                    //YET UNUSED
GLboolean upSideDown = false;                   //if we're upside down
GLboolean rainbow = true;                       //if we're using position to determine color, rather than brush color
GLboolean smooth = false;
GLboolean randomness = false;
int spray = 0;
const double PI = 4.0*atan(1.0);
float roomShade = .5;
int snakeSpeed = 50;

int voxelUse = 0;
int voxelTally = 0;

//Variables used to change the brush color
float startX, startY, startZ, startAng;
float startPosX, startPosZ, startDis;
float startr, startg, startb;
GLboolean colorChange = false;

//Current brush colors
float gred = .8;
float ggreen = .4;
float gblue = .1;
float galpha = 1;

float flicker = 1;                              //Flicker float (used to make block type 3 colors flicker)

typedef struct {                                //Voxel class
    float red;
    float green;
    float blue;
    float alpha;
    int start;
    int blockType;
    bool fertile;
}voxel;

typedef struct {                                //A color represented in RGB
    float r;
    float g;
    float b;
}rgbColor;

typedef struct {                                //A color represented by hue, saturation, and value
    float h;
    float s;
    float v;
}hsvColor;

//Storage units for converting colors to and from RGB/HSV
rgbColor rgbBase;
rgbColor conversion;
hsvColor hsvBase;


std::unordered_map <int, voxel> voxels;         //Stores all the voxels in the room.  very important!
std::unordered_map <int, voxel> drawnVoxels;
GLboolean voxelCheckList[resolution][resolution][resolution];
GLboolean voxelSecretList[resolution][resolution][resolution];
std::queue<int> toReproduce;
std::queue<int> toDie;
float size = 20./resolution;



//Camera/movement stuff

// camera direction angle
float angle = 0.0f;
float yangle = 0.0f;

// actual vector representing the camera's direction
float lx=0.0f,ly=0.0f,lz=-2;

// XYZ position of the camera
float x=10.0, y = 10., z=14.0;

//Voxel lock base position
float xlock= 0, ylock=0,zlock=0;

// the key states. These variables will be zero
//when no key is being pressed
float ydeltaAngle = 0.0;
float xdeltaAngle = 0.0;
float deltaMove = 0;
int xOrigin = -1;
int yOrigin = -1;

float deltaSide;



//------------------------------------------USEFUL FUNCTIONS-------------------

//VOXEL STUFF
//--------------Functions for voxel triangulation/creation/deletion

//reset all the spaces
void setUp(){
    voxels.clear();
    drawnVoxels.clear();
    
    //Also clear the voxel checklist
    for (int x = 0; x < resolution; x++){
        for (int y = 0; y < resolution; y++){
            for (int z = 0; z < resolution; z++){
                voxelCheckList[x][y][z] = false;
                voxelSecretList[x][y][z] = false;
            }
        }
    }
}

//Locate the nearest voxel based on lx/ly/lz (voxels cannot overlap)
void locksAVoxel(float x, float y, float z){
    xlock = (floor(((x + (proximity*lx))/size)+.5)) *size;
    ylock = (floor(((y + (proximity*ly))/size)+.5)) * size;
    zlock = (floor(((z + (proximity*lz))/size)+.5)) * size;
    
}

//Given coordinates, determines if this block is surrounded by other blocks
GLboolean closedIn(int x, int y, int z){
    GLboolean closed = true;
    //right
    if (x < resolution-1){
        if (!voxelCheckList[x+1][y][z]){
            closed = false;
        }
    }
    else{
        closed = false;
    }
    //left
    if (x > 0){
        if (!voxelCheckList[x-1][y][z]){
            closed = false;
        }
    }
    else{
        closed = false;
    }
    //top
    if (y < resolution-1){
        if (!voxelCheckList[x][y+1][z]){
            closed = false;
        }
    }
    else{
        closed = false;
    }
    //bottom
    if (y > 0){
        if (!voxelCheckList[x][y-1][z]){
            closed = false;
        }
    }
    else{
        closed = false;
    }
    //front
    if (z < resolution-1){
        if (!voxelCheckList[x][y][z+1]){
            closed = false;
        }
    }
    else{
        closed = false;
    }
    //back
    if (z > 0){
        if (!voxelCheckList[x][y][z-1]){
            closed = false;
        }
    }
    else{
        closed = false;
    }
    
    
    return closed;
}

//Given coordinates, updates the secret statuses of the voxel and its surrounding voxels
void updateVoxels(int i, int j, int k){
    voxelSecretList[i][j][k] = closedIn(i,j,k);
    if (voxelSecretList[i][j][k]){
        drawnVoxels.erase( (i*1000000)+(j*1000)+k);
    }
    
    if (i+1 < resolution){
        voxelSecretList[i+1][j][k] = closedIn(i+1,j,k);
        if (voxelSecretList[i+1][j][k]){
            drawnVoxels.erase( ((i+1)*1000000)+(j*1000)+k );
        }
    }
    
    if (i > 0){
        voxelSecretList[i-1][j][k] = closedIn(i-1,j,k);
        if (voxelSecretList[i-1][j][k]){
            drawnVoxels.erase( ((i-1)*1000000)+(j*1000)+k );
        }
    }
    
    if (j+1 < resolution){
        voxelSecretList[i][j+1][k] = closedIn(i,j+1,k);
        if (voxelSecretList[i][j+1][k]){
            drawnVoxels.erase( (i*1000000)+((j+1)*1000)+k );
        }
    }
    
    if (j > 0){
        voxelSecretList[i][j-1][k] = closedIn(i,j-1,k);
        if (voxelSecretList[i][j-1][k]){
            drawnVoxels.erase( (i*1000000)+((j-1)*1000)+k );
        }
    }
    
    if (k+1 < resolution){
        voxelSecretList[i][j][k+1] = closedIn(i,j,k+1);
        if (voxelSecretList[i][j][k+1]){
            drawnVoxels.erase( (i*1000000)+(j*1000)+ k + 1 );
        }
    }
    
    if (k > 0){
        voxelSecretList[i][j][k-1] = closedIn(i,j,k-1);
        if (voxelSecretList[i][j][k-1]){
            drawnVoxels.erase( (i*1000000)+(j*1000) + k - 1 );
        }
    }
}




//Draw a voxel given colors and alpha
void drawVoxel(float r, float g, float b, float a, int xind, int yind, int zind, GLboolean colorVoxel){
    
    if (smooth){
        glPushMatrix();
        glColor4f(r,g,b,a);
        float offset = size / 15;
        float pullback = size - offset;
        
        //back
        if (zind != 0){
            if (!voxelCheckList[xind][yind][zind-1] | colorVoxel){
                
                //            if (there's one to its back left){
                if (voxelCheckList[xind-1][yind][zind-1]){
                    glColor4f(r-.01,g-.01,b-.01,a);
                    glBegin(GL_QUADS);
                    glVertex3f(size,size,0.);
                    glVertex3f(0,size,-size);
                    glVertex3f(0,0,-size);
                    glVertex3f(size,0,0);
                    glEnd();
                    
                    if ( !voxelCheckList[xind][yind+1][zind-1] && !(voxelCheckList[xind][yind+1][zind] && voxelCheckList[xind-1][yind+1][zind-1] ) ){
                        glColor4f(r+.04,g+.04,b+.04,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(size,size,0);
                        glVertex3f(0,size,-size);
                        glVertex3f(0,size,0);
                        glEnd();
                    }
                    
                    if ( !voxelCheckList[xind][yind-1][zind-1] && !(voxelCheckList[xind][yind-1][zind] && voxelCheckList[xind-1][yind-1][zind-1] ) ){
                        glColor4f(r-.04,g-.04,b-.04,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(size,0,0);
                        glVertex3f(0,0,-size);
                        glVertex3f(0,0,0);
                        glEnd();
                    }
                }
                
                
                //FIX
                //If there is one behind and right
                if (voxelCheckList[xind+1][yind][zind-1]){
                    glColor4f(r-.04,g-.04,b-.04,a);
                    glBegin(GL_QUADS);
                    glVertex3f(0,size,0);
                    glVertex3f(size,size,-size);
                    glVertex3f(size,0,-size);
                    glVertex3f(0,0,0);
                    glEnd();
                    
                    
                    if ( !voxelCheckList[xind][yind+1][zind-1] && !(voxelCheckList[xind][yind+1][zind] && voxelCheckList[xind+1][yind+1][zind-1] ) ){
                        glColor4f(r-.01,g-.01,b-.01,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(0,size,0);
                        glVertex3f(size,size,-size);
                        glVertex3f(size,size,0);
                        glEnd();
                    }
                    
                    if ( !voxelCheckList[xind][yind-1][zind-1] && !(voxelCheckList[xind][yind-1][zind] && voxelCheckList[xind+1][yind-1][zind-1] ) ){
                        glColor4f(r-.06,g-.06,b-.06,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(0,0,0);
                        glVertex3f(size,0,-size);
                        glVertex3f(size,0,0);
                        glEnd();
                    }
                }
                
                
                //If there is one behind and below
                if (voxelCheckList[xind][yind-1][zind-1]){
                    glColor4f(r-.04,g-.04,b-.04,a);
                    glBegin(GL_QUADS);
                    glVertex3f(size,size,0.);
                    glVertex3f(0,size,0);
                    glVertex3f(0,0,-size);
                    glVertex3f(size,0,-size);
                    glEnd();
                    
                    if ( !voxelCheckList[xind-1][yind][zind-1] && !(voxelCheckList[xind-1][yind][zind] && voxelCheckList[xind-1][yind-1][zind-1] ) ){
                        glColor4f(r-.01,g-.01,b-.01,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(0,size,0);
                        glVertex3f(0,0,0);
                        glVertex3f(0,0,-size);
                        glEnd();
                    }
                    
                    if ( !voxelCheckList[xind+1][yind][zind-1] && !(voxelCheckList[xind+1][yind][zind] && voxelCheckList[xind+1][yind-1][zind-1] ) ){
                        glColor4f(r-.06,g-.06,b-.06,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(size,size,0);
                        glVertex3f(size,0,0);
                        glVertex3f(size,0,-size);
                        glEnd();
                    }
                }
                
                
                else if ( !(voxelCheckList[xind+1][yind][zind-1] | voxelCheckList[xind][yind+1][zind-1] | voxelCheckList[xind-1][yind][zind-1]) ){
                    glColor4f(r-.03,g-.03,b-.03,a);
                    glBegin(GL_QUADS);
                    glVertex3f(0.,0.,0.);
                    glVertex3f(size,0.,0.);
                    glVertex3f(size,size,0.);
                    glVertex3f(0.,size,0.);
                    glEnd();
                }
            }
        }
        else{
            glColor4f(r-.03,g-.03,b-.03,a);
            glBegin(GL_QUADS);
            glVertex3f(0.,0.,offset);
            glVertex3f(size,0.,offset);
            glVertex3f(size,size,offset);
            glVertex3f(0.,size,offset);
            glEnd();
        }
        //front
        if (zind != resolution - 1){
            if (!voxelCheckList[xind][yind][zind+1] | colorVoxel){
                
                
                //FIX
                //if there's a block under at the front
                if (voxelCheckList[xind][yind-1][zind+1]){
                    glColor4f(r+.04,g+.04,b+.04,a);
                    glBegin(GL_QUADS);
                    glVertex3f(0,size,size);
                    glVertex3f(size,size,size);
                    glVertex3f(size,0,2*size);
                    glVertex3f(0,0,2*size);
                    glEnd();
                    
                    if ( !voxelCheckList[xind-1][yind][zind+1] && !(voxelCheckList[xind-1][yind][zind] && voxelCheckList[xind-1][yind-1][zind+1] ) ){
                        glColor4f(r+.04,g+.04,b+.04,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(0,size,size);
                        glVertex3f(0,0,size);
                        glVertex3f(0,0,2*size);
                        glEnd();
                    }
                    
                    if ( !voxelCheckList[xind+1][yind][zind+1] && !(voxelCheckList[xind+1][yind][zind] && voxelCheckList[xind+1][yind-1][zind+1] ) ){
                        glColor4f(r-.04,g-.04,b-.04,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(size,size,size);
                        glVertex3f(size,0,size);
                        glVertex3f(size,0,2*size);
                        glEnd();
                    }
                }
                
                else if (! (voxelCheckList[xind+1][yind][zind+1] | voxelCheckList[xind-1][yind][zind+1] | voxelCheckList[xind][yind+1][zind+1])){
                    glBegin(GL_QUADS);
                    glColor4f(r+.03,g+.03,b+.03,a);
                    glVertex3f(0.,0.,size);
                    glVertex3f(size,0.,size);
                    glVertex3f(size,size,size);
                    glVertex3f(0.,size,size);
                    glEnd();
                }
            }
        }
        else{
            glBegin(GL_QUADS);
            glColor4f(r+.03,g+.03,b+.03,a);
            glVertex3f(0.,0.,pullback);
            glVertex3f(size,0.,pullback);
            glVertex3f(size,size,pullback);
            glVertex3f(0.,size,pullback);
            glEnd();
        }
        
        //left
        if (xind != 0){
            if (!voxelCheckList[xind-1][yind][zind] | colorVoxel){
                
                //if there's one on the back left
                if (voxelCheckList[xind-1][yind][zind-1]){
                    
                    glColor4f(r-.01,g-.01,b-.01,a);
                    glBegin(GL_QUADS);
                    glVertex3f(0,size,size);
                    glVertex3f(-size,size,0);
                    glVertex3f(-size,0,0);
                    glVertex3f(0,0,size);
                    glEnd();
                    
                    if ( !voxelCheckList[xind-1][yind+1][zind] && !(voxelCheckList[xind][yind+1][zind] && voxelCheckList[xind-1][yind+1][zind-1] ) ){
                        glColor4f(r+.04,g+.04,b+.04,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(0,size,size);
                        glVertex3f(0,size,0);
                        glVertex3f(-size,size,0);
                        glEnd();
                    }
                    
                    if ( !voxelCheckList[xind-1][yind-1][zind] && !(voxelCheckList[xind][yind-1][zind] && voxelCheckList[xind-1][yind-1][zind-1] ) ){
                        glColor4f(r-.04,g-.04,b-.04,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(0,0,size);
                        glVertex3f(0,0,0);
                        glVertex3f(-size,0,0);
                        glEnd();
                    }
                }
                
                
                //if there's one to the lower left
                if (voxelCheckList[xind-1][yind-1][zind]){
                    glColor4f(r+.04,g+.04,b+.04,a);
                    glBegin(GL_QUADS);
                    glVertex3f(0,size,0);
                    glVertex3f(-size,0,0);
                    glVertex3f(-size,0,size);
                    glVertex3f(0,size,size);
                    glEnd();
                    
                    if ( !voxelCheckList[xind-1][yind][zind-1] && !(voxelCheckList[xind][yind][zind-1] && voxelCheckList[xind-1][yind-1][zind-1]) ){
                        glColor4f(r-.02,g-.02,b-.02,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(0,size,0);
                        glVertex3f(-size,0,0);
                        glVertex3f(0,0,0);
                        glEnd();
                    }
                    
                    if ( !voxelCheckList[xind-1][yind][zind+1] && !(voxelCheckList[xind][yind][zind+1] && voxelCheckList[xind-1][yind-1][zind+1]) ){
                        glColor4f(r+.02,g+.02,b+.02,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(0,size,size);
                        glVertex3f(-size,0,size);
                        glVertex3f(0,0,size);
                        glEnd();
                    }
                }
                
                
                else if ( !( voxelCheckList[xind-1][yind][zind+1] | voxelCheckList[xind-1][yind][zind-1] | voxelCheckList[xind-1][yind+1][zind] )){
                    glColor4f(r,g,b,a);
                    glBegin(GL_QUADS);
                    glVertex3f(0.,0.,0.);
                    glVertex3f(0.,0.,size);
                    glVertex3f(0.,size,size);
                    glVertex3f(0.,size,0.);
                    glEnd();
                }
            }
        }
        else{
            glColor4f(r,g,b,a);
            glBegin(GL_QUADS);
            glVertex3f(offset,0.,0.);
            glVertex3f(offset,0.,size);
            glVertex3f(offset,size,size);
            glVertex3f(offset,size,0.);
            glEnd();
        }
        //right
        if (xind != resolution - 1){
            if (!voxelCheckList[xind+1][yind][zind] | colorVoxel){
                
                
                //FIX
                //If there is one to the lower right
                if (voxelCheckList[xind+1][yind-1][zind]){
                    glColor4f(r-.04,g-.04,b-.04,a);
                    glBegin(GL_QUADS);
                    glVertex3f(size,size,size);
                    glVertex3f(size,size,0);
                    glVertex3f(2*size,0,0);
                    glVertex3f(2*size,0,size);
                    glEnd();
                    
                    
                    if ( !voxelCheckList[xind+1][yind][zind-1] && !(voxelCheckList[xind][yind][zind-1] && voxelCheckList[xind+1][yind-1][zind-1] ) ){
                        glColor4f(r-.01,g-.01,b-.01,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(size,size,0);
                        glVertex3f(2*size,0,0);
                        glVertex3f(size,0,0);
                        glEnd();
                    }
                    
                    if ( !voxelCheckList[xind+1][yind][zind+1] && !(voxelCheckList[xind][yind][zind+1] && voxelCheckList[xind+1][yind-1][zind+1] ) ){
                        glColor4f(r-.06,g-.06,b-.06,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(size,size,size);
                        glVertex3f(2*size,0,size);
                        glVertex3f(size,0,size);
                        glEnd();
                    }
                }
                
                //FIX
                //If there is one to the back right
                if (voxelCheckList[xind+1][yind][zind-1]){
                    glColor4f(r-.04,g-.04,b-.04,a);
                    glBegin(GL_QUADS);
                    glVertex3f(size,size,size);
                    glVertex3f(2*size,size,0);
                    glVertex3f(2*size,0,0);
                    glVertex3f(size,0,size);
                    glEnd();
                    
                    
                    if ( !voxelCheckList[xind+1][yind+1][zind] && !(voxelCheckList[xind][yind+1][zind] && voxelCheckList[xind+1][yind+1][zind-1] ) ){
                        glColor4f(r-.01,g-.01,b-.01,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(size,size,size);
                        glVertex3f(size,size,0);
                        glVertex3f(2*size,size,0);
                        glEnd();
                    }
                    
                    if ( !voxelCheckList[xind+1][yind-1][zind] && !(voxelCheckList[xind][yind-1][zind] && voxelCheckList[xind+1][yind-1][zind-1] ) ){
                        glColor4f(r-.06,g-.06,b-.06,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(size,0,size);
                        glVertex3f(size,0,0);
                        glVertex3f(2*size,0,0);
                        glEnd();
                    }
                }
                
                
                
                else if (!(voxelCheckList[xind+1][yind][zind+1] | voxelCheckList[xind+1][yind+1][zind] | voxelCheckList[xind+1][yind-1][zind]) ){
                    glColor4f(r,g,b,a);
                    glBegin(GL_QUADS);
                    glVertex3f(size,0.,0.);
                    glVertex3f(size,0.,size);
                    glVertex3f(size,size,size);
                    glVertex3f(size,size,0.);
                    glEnd();
                }
            }
        }
        else{
            glColor4f(r,g,b,a);
            glBegin(GL_QUADS);
            glVertex3f(pullback,0.,0.);
            glVertex3f(pullback,0.,size);
            glVertex3f(pullback,size,size);
            glVertex3f(pullback,size,0.);
            glEnd();
        }
        //top
        if (yind != resolution - 1){
            if ( !(voxelCheckList[xind][yind+1][zind] | voxelCheckList[xind][yind+1][zind+1] | voxelCheckList[xind][yind+1][zind-1] | voxelCheckList[xind+1][yind+1][zind] | voxelCheckList[xind-1][yind+1][zind])| colorVoxel){
                glColor4f(r+.05,g+.05,b+.05,a);
                glBegin(GL_QUADS);
                glVertex3f(0.,size,0.);
                glVertex3f(0.,size,size);
                glVertex3f(size,size,size);
                glVertex3f(size,size,0.);
                glEnd();
            }
        }
        else{
            glColor4f(r+.05,g+.05,b+.05,a);
            glBegin(GL_QUADS);
            glVertex3f(0.,pullback,0.);
            glVertex3f(0.,pullback,size);
            glVertex3f(size,pullback,size);
            glVertex3f(size,pullback,0.);
            glEnd();
        }
        //bottom
        if (yind != 0){
            if (!voxelCheckList[xind][yind-1][zind] | colorVoxel){
                
                //if there's one to the lower left
                if (voxelCheckList[xind-1][yind-1][zind]){
                    glColor4f(r-.04,g-.04,b-.04,a);
                    glBegin(GL_QUADS);
                    glVertex3f(size,0,0);
                    glVertex3f(0,-size,0);
                    glVertex3f(0,-size,size);
                    glVertex3f(size,0,size);
                    glEnd();
                    
                    
                    if ( !voxelCheckList[xind][yind-1][zind-1] && !(voxelCheckList[xind][yind][zind-1] && voxelCheckList[xind-1][yind-1][zind-1] ) ){
                        glColor4f(r-.04,g-.04,b-.04,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(size,0,0);
                        glVertex3f(0,0,0);
                        glVertex3f(0,-size,0);
                        glEnd();
                    }
                    
                    if ( !voxelCheckList[xind][yind-1][zind+1] && !(voxelCheckList[xind][yind][zind+1] && voxelCheckList[xind-1][yind-1][zind+1] ) ){
                        glColor4f(r+.02,g+.02,b+.02,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(size,0,size);
                        glVertex3f(0,0,size);
                        glVertex3f(0,-size,size);
                        glEnd();
                    }
                }
                
                //if there's one to lower rear
                if (voxelCheckList[xind][yind-1][zind-1]){
                    glColor4f(r-.06,g-.06,b-.06,a);
                    glBegin(GL_QUADS);
                    glVertex3f(size,0,size);
                    glVertex3f(0,0,size);
                    glVertex3f(0,-size,0);
                    glVertex3f(size,-size,0);
                    glEnd();
                    
                    
                    if ( !voxelCheckList[xind-1][yind-1][zind] && !(voxelCheckList[xind-1][yind][zind] && voxelCheckList[xind-1][yind-1][zind-1] ) ){
                        glColor4f(r-.01,g-.01,b-.01,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(0,0,size);
                        glVertex3f(0,0,0);
                        glVertex3f(0,-size,0);
                        glEnd();
                    }
                    
                    if ( !voxelCheckList[xind+1][yind-1][zind] && !(voxelCheckList[xind+1][yind][zind] && voxelCheckList[xind+1][yind-1][zind-1] ) ){
                        glColor4f(r+.04,g+.04,b+.04,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(size,0,size);
                        glVertex3f(size,0,0);
                        glVertex3f(size,-size,0);
                        glEnd();
                    }
                }
                
                //FIX
                //If there is one to the lower right
                if (voxelCheckList[xind+1][yind-1][zind]){
                    glColor4f(r-.04,g-.04,b-.04,a);
                    glBegin(GL_QUADS);
                    glVertex3f(0,0,size);
                    glVertex3f(0,0,0);
                    glVertex3f(size,-size,0);
                    glVertex3f(size,-size,size);
                    glEnd();
                    
                    if ( !voxelCheckList[xind][yind-1][zind+1] && !(voxelCheckList[xind][yind][zind+1] && voxelCheckList[xind+1][yind-1][zind+1] ) ){
                        glColor4f(r-.01,g-.01,b-.01,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(0,0,size);
                        glVertex3f(size,-size,size);
                        glVertex3f(size,0,size);
                        glEnd();
                    }
                    
                    if ( !voxelCheckList[xind][yind-1][zind-1] && !(voxelCheckList[xind][yind][zind-1] && voxelCheckList[xind+1][yind-1][zind-1] ) ){
                        glColor4f(r-.06,g-.06,b-.06,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(0,0,0);
                        glVertex3f(size,-size,0);
                        glVertex3f(size,0,0);
                        glEnd();
                    }
                }
                
                
                //If there is one to the lower front
                if (voxelCheckList[xind][yind-1][zind+1]){
                    glColor4f(r-.04,g-.04,b-.04,a);
                    glBegin(GL_QUADS);
                    glVertex3f(0,0,0.);
                    glVertex3f(size,0,0);
                    glVertex3f(size,-size,size);
                    glVertex3f(0,-size,size);
                    glEnd();
                    
                    
                    if ( !voxelCheckList[xind-1][yind-1][zind] && !(voxelCheckList[xind-1][yind][zind] && voxelCheckList[xind-1][yind-1][zind+1] ) ){
                        glColor4f(r-.01,g-.01,b-.01,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(0,0,0);
                        glVertex3f(0,0,size);
                        glVertex3f(0,-size,size);
                        glEnd();
                    }
                    
                    if ( !voxelCheckList[xind+1][yind-1][zind] && !(voxelCheckList[xind+1][yind][zind] && voxelCheckList[xind+1][yind-1][zind+1] ) ){
                        glColor4f(r-.06,g-.06,b-.06,a);
                        glBegin(GL_TRIANGLES);
                        glVertex3f(size,0,0);
                        glVertex3f(size,0,size);
                        glVertex3f(size,-size,size);
                        glEnd();
                    }
                }
                
                
                else if ( ! (voxelCheckList[xind-1][yind-1][zind] | voxelCheckList[xind][yind-1][zind-1] | voxelCheckList[xind+1][yind-1][zind]) ){
                    glColor4f(r-.05,g-.05,b-.05,a);
                    glBegin(GL_QUADS);
                    glVertex3f(0.,0,0.);
                    glVertex3f(0.,0,size);
                    glVertex3f(size,0,size);
                    glVertex3f(size,0,0.);
                    glEnd();
                }
            }
        }
        else{
            glColor4f(r-.05,g-.05,b-.05,a);
            glBegin(GL_QUADS);
            glVertex3f(0.,offset,0.);
            glVertex3f(0.,offset,size);
            glVertex3f(size,offset,size);
            glVertex3f(size,offset,0.);
            glEnd();
        }
        glPopMatrix();
    }
    
    
    
    
    
    
    else{
        glPushMatrix();
        glColor4f(r,g,b,a);
        float offset = size / 15;
        float pullback = size - offset;
        
        //back
        if (zind != 0){
            if (!voxelCheckList[xind][yind][zind-1] | colorVoxel){
                glColor4f(r-.03,g-.03,b-.03,a);
                glBegin(GL_QUADS);
                glVertex3f(0.,0.,0.);
                glVertex3f(size,0.,0.);
                glVertex3f(size,size,0.);
                glVertex3f(0.,size,0.);
                glEnd();
            }
        }
        else{
            glColor4f(r-.03,g-.03,b-.03,a);
            glBegin(GL_QUADS);
            glVertex3f(0.,0.,offset);
            glVertex3f(size,0.,offset);
            glVertex3f(size,size,offset);
            glVertex3f(0.,size,offset);
            glEnd();
        }
        //front
        if (zind != resolution - 1){
            if (!voxelCheckList[xind][yind][zind+1] | colorVoxel){
                glBegin(GL_QUADS);
                glColor4f(r+.03,g+.03,b+.03,a);
                glVertex3f(0.,0.,size);
                glVertex3f(size,0.,size);
                glVertex3f(size,size,size);
                glVertex3f(0.,size,size);
                glEnd();
            }
        }
        else{
            glBegin(GL_QUADS);
            glColor4f(r+.03,g+.03,b+.03,a);
            glVertex3f(0.,0.,pullback);
            glVertex3f(size,0.,pullback);
            glVertex3f(size,size,pullback);
            glVertex3f(0.,size,pullback);
            glEnd();
        }
        //left
        if (xind != 0){
            if (!voxelCheckList[xind-1][yind][zind] | colorVoxel){
                glColor4f(r,g,b,a);
                glBegin(GL_QUADS);
                glVertex3f(0.,0.,0.);
                glVertex3f(0.,0.,size);
                glVertex3f(0.,size,size);
                glVertex3f(0.,size,0.);
                glEnd();
            }
        }
        else{
            glColor4f(r,g,b,a);
            glBegin(GL_QUADS);
            glVertex3f(offset,0.,0.);
            glVertex3f(offset,0.,size);
            glVertex3f(offset,size,size);
            glVertex3f(offset,size,0.);
            glEnd();
        }
        //right
        if (xind != resolution - 1){
            if (!voxelCheckList[xind+1][yind][zind] | colorVoxel){
                glColor4f(r,g,b,a);
                glBegin(GL_QUADS);
                glVertex3f(size,0.,0.);
                glVertex3f(size,0.,size);
                glVertex3f(size,size,size);
                glVertex3f(size,size,0.);
                glEnd();
            }
        }
        else{
            glColor4f(r,g,b,a);
            glBegin(GL_QUADS);
            glVertex3f(pullback,0.,0.);
            glVertex3f(pullback,0.,size);
            glVertex3f(pullback,size,size);
            glVertex3f(pullback,size,0.);
            glEnd();
        }
        //top
        if (yind != resolution - 1){
            if (!voxelCheckList[xind][yind+1][zind] | colorVoxel){
                glColor4f(r+.05,g+.05,b+.05,a);
                glBegin(GL_QUADS);
                glVertex3f(0.,size,0.);
                glVertex3f(0.,size,size);
                glVertex3f(size,size,size);
                glVertex3f(size,size,0.);
                glEnd();
            }
        }
        else{
            glColor4f(r+.05,g+.05,b+.05,a);
            glBegin(GL_QUADS);
            glVertex3f(0.,pullback,0.);
            glVertex3f(0.,pullback,size);
            glVertex3f(size,pullback,size);
            glVertex3f(size,pullback,0.);
            glEnd();
        }
        //bottom
        if (yind != 0){
            if (!voxelCheckList[xind][yind-1][zind] | colorVoxel){
                glColor4f(r-.05,g-.05,b-.05,a);
                glBegin(GL_QUADS);
                glVertex3f(0.,0,0.);
                glVertex3f(0.,0,size);
                glVertex3f(size,0,size);
                glVertex3f(size,0,0.);
                glEnd();
            }
        }
        else{
            glColor4f(r-.05,g-.05,b-.05,a);
            glBegin(GL_QUADS);
            glVertex3f(0.,offset,0.);
            glVertex3f(0.,offset,size);
            glVertex3f(size,offset,size);
            glVertex3f(size,offset,0.);
            glEnd();
        }
        glPopMatrix();
    }
}

//Draw a black voxel outline given x, y, and z camera positions
void drawOutline(float x, float y, float z, GLboolean safe){
    glPushMatrix();
    
    glColor3f(0,0,1);
    if (!safe){
        glColor3f(1,0,0);
    }
    //back
    glBegin(GL_LINES);
    glVertex3d(x,y,z);
    glVertex3d(x+size,y,z);
    
    glVertex3d(x,y,z+size);
    glVertex3d(x+size,y,z+size);
    
    glVertex3d(x,y+size,z);
    glVertex3d(x+size,y+size,z);
    
    glVertex3d(x,y+size,z+size);
    glVertex3d(x+size,y+size,z+size);
    
    glVertex3d(x,y,z);
    glVertex3d(x,y+size,z);
    
    glVertex3d(x,y,z+size);
    glVertex3d(x,y+size,z+size);
    
    glVertex3d(x+size,y,z);
    glVertex3d(x+size,y+size,z);
    
    glVertex3d(x+size,y,z+size);
    glVertex3d(x+size,y+size,z+size);
    
    glVertex3d(x,y,z);
    glVertex3d(x,y,z+size);
    
    glVertex3d(x+size,y,z);
    glVertex3d(x+size,y,z+size);
    
    glVertex3d(x,y+size,z);
    glVertex3d(x,y+size,z+size);
    
    glVertex3d(x+size,y+size,z);
    glVertex3d(x+size,y+size,z+size);
    
    glEnd();
    
    glPopMatrix();
}



//BUILDING WALLS
//--------------Functions for building the walls

//Draw a wall
void drawWall(GLboolean ceiling) {
    glBegin(GL_QUADS);
    glVertex3f(20, 20, 0);
    glVertex3f(0, 20, 0);
    glColor3f((50./255.*roomShade),(45./255*roomShade),(60./255.*roomShade));
    if (ceiling){
        glColor3f(roomShade*.8,roomShade*.8,roomShade*.8);
    }
    glVertex3f(0,0,0);
    glVertex3f(20,0,0);
    glEnd();
}

//Draw the grid for the walls
void drawGrid(){
    glPushMatrix();
    if (size/13 > .02){
        glTranslatef(0.,0.,size/13);
    }
    else{
        glTranslatef(0.,0.,.02);
    }
    glBegin(GL_LINES);
    glColor4f(1.,1.,1.,.1);
    for(int i=1; i < 20; i +=1){
        glVertex2f(0, i);
        glVertex2f(20,i);
    }
    for(int i=1; i < 20; i+=1){
        glVertex2f(i, 0);
        glVertex2f(i, 20);
    }
    
    glEnd();
    glPopMatrix();
}



//COLOR STUFF
//--------------Color setting and conversions (rgb->hsv and hsv->rgb code comes from the web)

//Take a color from RGB, spit out the same color in HSV (I got this code online)
hsvColor rgb_to_hsv(rgbColor rgb)
{
	float Max;
	float Min;
	float chroma;
	hsvColor hsv;
    
    Min = std::min(std::min(rgb.r,rgb.g),rgb.b);
    Max = std::max(std::max(rgb.r,rgb.g),rgb.b);
	chroma = Max - Min;
    
	//If Chroma is 0, then S is 0 by definition, and H is undefined but 0 by convention.
	if(chroma != 0)
	{
		if(rgb.r == Max)
		{
			hsv.h = (rgb.g - rgb.b) / chroma;
            
			if(hsv.h < 0.0)
			{
				hsv.h += 6.0;
			}
		}
		else if(rgb.g == Max)
		{
			hsv.h = ((rgb.b - rgb.r) / chroma) + 2.0;
		}
		else //rgb.b == Max
		{
			hsv.h = ((rgb.r - rgb.g) / chroma) + 4.0;
		}
        
		hsv.h *= 60.0;
		hsv.s = chroma / Max;
	}
    
	hsv.v = Max;
    
	return hsv;
}

//Take a color from HSV, spit out the same color in RGB (I got this code online)
rgbColor hsv_to_rgb(float h, float s, float v)
{
	float Min;
	float chroma;
	float hDash;
	float x;
	rgbColor rgb;
    
	chroma = s * v;
	hDash = h / 60.0;
    
	x = chroma * (1.0 - fabs( fmod(hDash, 2.0) - 1.0));
    
	if(hDash < 1.0)
	{
		rgb.r = chroma;
		rgb.g = x;
	}
	else if(hDash < 2.0)
	{
		rgb.r = x;
		rgb.g = chroma;
	}
	else if(hDash < 3.0)
	{
		rgb.g = chroma;
		rgb.b = x;
	}
	else if(hDash < 4.0)
	{
		rgb.g= x;
		rgb.b = chroma;
	}
	else if(hDash < 5.0)
	{
		rgb.r = x;
		rgb.b = chroma;
	}
	else if(hDash <= 6.0)
	{
		rgb.r = chroma;
		rgb.b = x;
	}
    
	Min = v - chroma;
    
	rgb.r += Min;
	rgb.g += Min;
	rgb.b += Min;
    
	return rgb;
}

//Set up the global variables to start changing colors, called when you first press 'c'
void colorSet(float xStart, float yStart, float zStart, float r, float g, float b){
    startX = xStart;
    startY = yStart;
    startZ = zStart;
    startDis =  pow( pow((z-startZ),2) + pow((x - startX),2), .5);
    
    rgbBase = rgbColor {r, g, b};
    hsvBase = rgb_to_hsv(rgbBase);
}


//POSITIONING
//--------------Setting the camera and changing the window's size

//change window
void changeSize(int w, int h) {
    
	// Prevent a divide by zero, when window is too short
	if (h == 0)
		h = 1;
    
	float ratio =  w * 1.0 / h;
    
	// Use the Projection Matrix
	glMatrixMode(GL_PROJECTION);
    
	// Reset Matrix
	glLoadIdentity();
    
	// Set the viewport to be the entire window
	glViewport(0, 0, w, h);
    
	// Set the correct perspective.
	gluPerspective(45.0f, ratio, 0.1, 100.0f);
    
	// Get Back to the Modelview
	glMatrixMode(GL_MODELVIEW);
}

//Calculate next position
void computePos(float deltaMove, float deltaSide) {
    
    y += deltaMove * ly * 0.1;
    
    x += deltaMove * lx * 0.1;
    
    z += deltaMove * lz * 0.1;
    
    if (!upSideDown){
        x += deltaSide * (-lz) * 0.1;
        
        z += deltaSide * (lx) * 0.1;
    }
    else{
        
        x -= deltaSide * (-lz) * 0.1;
        
        z -= deltaSide * (lx) * 0.1;
    }
    
    
    if (x >=19.5)
        x = 19.49;
    if (x <=.5)
        x = .51;
    
    if (y >=19.5)
        y = 19.49;
    if (y <=.5)
        y = .51;
    
    if (z >=19.5)
        z = 19.49;
    if (z <=.5)
        z = .51;
}

//Calculate direction based on keyboard/mouse input
//(Update angle/yangle and lx/ly/lz
void computeDir(float xdeltaAngle, float ydeltaAngle) {
    
    if (!upSideDown)
        angle += xdeltaAngle*.25;
    else
        angle -= xdeltaAngle*.25;
    
    yangle += ydeltaAngle*.25;
    
    if (cos(yangle) <0){
        upSideDown = true;
    }
    else{
        upSideDown = false;
    }
    
	lx = 2*sin(angle)*cos(yangle);
    ly = -2*sin(yangle);
	lz = -2*cos(angle)*cos(yangle);
}







//------------------------------------------------MAIN LOOP---------------------------

void renderScene(void) {
    
    
    //    glEnable(GL_LIGHTING);
    //    glEnable(GL_LIGHT0);
    //    glEnable(GL_COLOR_MATERIAL);
    //    GLfloat light_ambient[] = {.1,.1,.1,1.};
    //    GLfloat light_position[] = {-1,1,-1,0};
    //    glLightfv(GL_LIGHT0,GL_AMBIENT, light_ambient);
    //    glLightfv(GL_LIGHT0,GL_POSITION,light_position);
    
    computePos(deltaMove, deltaSide);                           //reposition the camera
    
    if (xdeltaAngle or ydeltaAngle)
		computeDir(xdeltaAngle, ydeltaAngle);                   //redirect the camera if needed
    
	// Clear Color and Depth Buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    locksAVoxel(x,y,z);                                         //Position the brush (set xlock/ylock/zlock)
    
    
    //Set the flicker value for blocks of type 3.
    {
        int movement = rand()%5;
        if (movement == 0){
            flicker -=.05;
            if (flicker <-.7){
                flicker = -.7;
            }
        }
        if (movement == 1){
            flicker += .05;
            if (flicker > .2){
                flicker = .2;
            }
        }
    }
    
    
	// Reset transformations
	glLoadIdentity();
    
	// Set the camera
    {
        if (!upSideDown){
            gluLookAt(	x, y, z,
                      x+lx, y+ly, z+lz,
                      .0, 1.0, 0.0);
        }
        else{
            gluLookAt(x,y,z,
                      x+lx,y+ly,z+lz,
                      0,-1,0);
        }
    }
    
    
    
    //Drawing the building
    {
        //FIRST THE WALLS
        //LATER, THE GRID
        
        //Floor
        glPushMatrix();
        glTranslatef(0,0,20);
        glRotatef(-90,1,0,0);
        glColor3f((roomShade*50/255.),(roomShade*45/255.),(roomShade*60/255.));
        drawWall(false);
        glPopMatrix();
        
        //Ceiling
        glPushMatrix();
        glRotatef(90,1,0,0);
        glTranslatef(0,0,-20);
        glColor3f(roomShade*.8,roomShade*.8,roomShade*.8);
        drawWall(true);
        glPopMatrix();
        
        //Front
        glColor3f(roomShade*.8,roomShade*.8,roomShade*.8);
        drawWall(false);
        
        //Back
        glPushMatrix();
        glTranslatef(20,0,20);
        glRotatef(180,0,1,0);
        glColor3f(roomShade*.8,roomShade*.8,roomShade*.8);
        drawWall(false);
        glPopMatrix();
        
        //Left
        glPushMatrix();
        glTranslatef(0,0,20);
        glRotatef(90,0,1,0);
        glColor3f(roomShade*.8,roomShade*.8,roomShade*.8);
        drawWall(false);
        glPopMatrix();
        
        //Right
        glPushMatrix();
        glTranslatef(20,0,0);
        glRotatef(-90,0,1,0);
        glColor3f(roomShade*.8,roomShade*.8,roomShade*.8);
        drawWall(false);
        glPopMatrix();
    }
    
    
    
    
    
    //-----------------CHANGING THE BRUSH COLOR
    //If we're holding 'c' to change the color, do this stuff:
    if (colorChange){
        //modify base HSV using position, then convert those values to RGB again
        
        //Calculate angle change for hue change
        float angDiff = fmod(4*(angle - startAng)/(2*PI)*360,360);
        float hue = 0;
        if (angDiff + hsvBase.h <= 0){
            hue = 360 + (angDiff + hsvBase.h);
        }
        else{
            hue = fmod(hsvBase.h + angDiff,360);
        }
        
        //Calculate forward motion for saturation change
        float newDis = pow(pow(startPosX-(x + lx),2) + pow(startPosZ - (z + lz),2),.5);
        float deltaDis = (newDis - startDis)/1.5;
        float saturation = hsvBase.s + deltaDis;
        
        
        //Calculate up/down motion for value change
        float ymov = ((y + ly) - startY)/2;
        float value = ymov + hsvBase.v;
        if (value > 1){
            value = 1;
        }
        
        
        
        conversion = hsv_to_rgb(hue,saturation,value);      //This is the new RGB value
        
        //Setting the converted RGB values to the brush colors gred ggreen and gblue
        gred = conversion.r;
        if (gred < 0){
            gred = 0;
        }
        if (gred > 1){
            gred = 1;
        }
        ggreen = conversion.g;
        if (ggreen < 0){
            ggreen = 0;
        }
        if (ggreen > 1){
            ggreen = 1;
        }
        gblue = conversion.b;
        if (gblue < 0){
            gblue = 0;
        }
        if (gblue > 1){
            gblue = 1;
        }
        
        
        
        glPushMatrix();
        glTranslatef(x+lx,y+ly,z+lz);
        drawVoxel(gred,ggreen,gblue,1,1,1,1,true);
        glPopMatrix();
    }
    //Otherwise make sure you show the target voxel outline:
    else{
        if (spray == 1){
            GLboolean contact = false;
            float distance = size;
            while (contact == false){
                
                int sprayX = floor( (( (x + (distance*lx)) /size)+.5) );
                int sprayY = floor( (( (y + (distance*ly)) /size)+.5) );
                int sprayZ = floor( (( (z + (distance*lz)) /size)+.5) );
                
                if (sprayX > resolution | sprayX < 0 | sprayY > resolution | sprayY < 0 | sprayZ > resolution | sprayZ <0){
                    contact = true;
                    break;
                }
                
                
                if (voxelCheckList[sprayX][sprayY][sprayZ] && !voxelSecretList[sprayX][sprayY][sprayZ]){
                    //if you're positioned INSIDE a voxel, make the brush outline red
                    if (voxelCheckList[int(floor((x/size)+.5))][int(floor((y/size)+.5))][int(floor((z/size)+.5))]){
                        drawOutline(sprayX*size,sprayY*size,sprayZ*size,false);
                    }
                    //otherwise it should be blue
                    else{
                        drawOutline(sprayX*size,sprayY*size,sprayZ*size,true);
                    }
                    contact = true;
                }
                distance += size/10;
            }
            
        }
        
        else{
            //if your head is inside a voxel, the brush outline is red
            if (voxelCheckList[int(floor((x/size)+.5))][int(floor((y/size)+.5))][int(floor((z/size)+.5))]){
                drawOutline(xlock,ylock,zlock,false);
            }
            //otherwise, it's blue
            else{
                drawOutline(xlock,ylock,zlock,true);
            }
        }
    }
    
    
    
    
    //-----------------PLACING VOXELS
    //if draw is selected, add the appropriate voxels to the voxelmap to be drawn from now on.
    
    if (draw){
        
        int xloc = floor((xlock/size)+.5);
        int yloc = floor((ylock/size)+.5);
        int zloc = floor((zlock/size)+.5);
        
        
        
        //If we're spraying
        if (spray == 1){
            
            int xpos = floor((x/size)+.5);
            int ypos = floor((y/size)+.5);
            int zpos = floor((z/size)+.5);
            
            GLboolean contact = false;
            float distance = size;
            while (contact == false){
                
                int sprayX = floor( (( (x + (distance*lx)) /size)+.5) );
                int sprayY = floor( (( (y + (distance*ly)) /size)+.5) );
                int sprayZ = floor( (( (z + (distance*lz)) /size)+.5) );
                
                if (sprayX > resolution | sprayX < 0 | sprayY > resolution | sprayY < 0 | sprayZ > resolution | sprayZ <0){
                    break;
                }
                
                
                if ((voxelCheckList[sprayX][sprayY][sprayZ] && !voxelSecretList[sprayX][sprayY][sprayZ])){
                    
                    //color that voxel, its friends, then contact = true;
                    int time = glutGet(GLUT_ELAPSED_TIME);
                    
                    //spread = radius of how wide the spray paint area should be
                    float spread = sprayDiameter/4+distance / 3.;
                    
                    for (int i = sprayX-int(spread); i < sprayX+int(spread)+1; i++){
                        for (int j = sprayY-int(spread); j < sprayY+int(spread)+1; j++){
                            for (int k = sprayZ-int(spread); k < sprayZ+int(spread)+1; k++){
                                
                                //Distance is how far the current voxel is from the center voxel
                                float distFromCenter = pow(pow(sprayY-j,2)+pow(sprayX-i,2)+pow(sprayZ-k,2),.5);
                                //This says 'if the voxel's distance from center voxel == diameter' (so diameter is more a radius)
                                //and is responsible for leaving the insides of large brushes empty.  only makes shells.
                                if (distFromCenter <= spread){
                                    
                                    GLboolean oContact = false;
                                    float oDistance = size;
                                    while (oContact == false){
                                        
                                        float xdif = size*(i-xpos);
                                        float ydif = size*(j-ypos);
                                        float zdif = size*(k-zpos);
                                        
                                        
                                        int distX = floor(((x + (oDistance*xdif))/size)+.5);
                                        int distY = floor(((y + (oDistance*ydif))/size)+.5);
                                        int distZ = floor(((z + (oDistance*zdif))/size)+.5);
                                        
                                        
                                        if (distX > resolution | distX < 0 | distY > resolution | distY < 0 | distZ > resolution | distZ <0){
                                            break;
                                        }
                                        
                                        
                                        if ((voxelCheckList[distX][distY][distZ] && !voxelSecretList[distX][distY][distZ])){
                                            
                                            
                                            if (!rainbow){                  //If not rainbow, use the brush colors.
                                                //Here we actually put the voxel on the map. (do it a few lines down if rainbow mode)
                                                if (!randomness){
                                                    voxels[(distX*1000000)+(distY*1000)+distZ] = {gred,ggreen,gblue,galpha,time,blockPlaceType,false};
                                                    drawnVoxels[(distX*1000000)+(distY*1000)+distZ] = {gred,ggreen,gblue,galpha,time,blockPlaceType,false};
                                                }
                                                else{
                                                    voxels[(distX*1000000)+(distY*1000)+distZ] = {static_cast<float>((rand()%100)/100.),static_cast<float>((rand()%100)/100.),static_cast<float>((rand()%100)/100.),galpha,time,blockPlaceType,false};
                                                    drawnVoxels[(distX*1000000)+(distY*1000)+distZ] = {gred,ggreen,gblue,galpha,time,blockPlaceType,false};
                                                }
                                            }
                                            else{                           //If rainbow, use the voxel position for its color
                                                voxels[(distX*1000000)+(distY*1000)+distZ] = {distX*size/20,distY*size/20,distZ*size/20,galpha,time,blockPlaceType,false};
                                                drawnVoxels[(distX*1000000)+(distY*1000)+distZ] = {distX*size/20,distY*size/20,distZ*size/20,galpha,time,blockPlaceType,false};
                                            }
                                            
                                            if (voxels[(distX*1000000)+(distY*1000)+distZ].blockType == 4){
                                                voxels[(distX*1000000)+(distY*1000)+distZ].fertile = drawnVoxels[(distX*1000000)+(distY*1000)+distZ].fertile = true;
                                            }
                                            oContact = true;
                                            
                                            
                                        }
                                        if (voxelCheckList[distX][distY][distZ]){
                                            oContact = true;
                                        }
                                        
                                        oDistance += size/2;
                                    }
                                    
                                }
                            }
                        }
                    }
                    
                    contact = true;
                }
                
                if (voxelCheckList[sprayX][sprayY][sprayZ] && voxelSecretList[sprayX][sprayY][sprayZ]){
                    contact = true;
                }
                
                distance+= size/2;
                
            }
        }
        
        //if we're painting on structures
        if (spray == 2){
            
            int time = glutGet(GLUT_ELAPSED_TIME);
            
            int xloc = floor((xlock/size)+.5);
            int yloc = floor((ylock/size)+.5);
            int zloc = floor((zlock/size)+.5);
            
            float spread = sprayDiameter/2;
            
            
            for (int i = xloc-(int)spread; i< xloc+int(spread)+1; i++){
                for (int j = yloc-(int)spread; j< yloc+int(spread)+1; j++){
                    for (int k = zloc-(int)spread; k< zloc+int(spread)+1; k++){
                        
                        float distFromCenter = pow(pow(xloc-i,2)+pow(yloc-j,2)+pow(zloc-k,2),.5);
                        
                        if (distFromCenter <= spread){
                            
                            
                            if (i > resolution | i < 0 | j > resolution | j < 0 | k > resolution | k <0){
                                break;
                            }
                            
                            
                            if ((voxelCheckList[i][j][k] && !voxelSecretList[i][j][k])){
                                
                                if (!rainbow){                  //If not rainbow, use the brush colors.
                                    //Here we actually put the voxel on the map. (do it a few lines down if rainbow mode)
                                    if (!randomness){
                                        voxels[(i*1000000)+(j*1000)+k] = {gred,ggreen,gblue,galpha,time,blockPlaceType,false};
                                        drawnVoxels[(i*1000000)+(j*1000)+k] = {gred,ggreen,gblue,galpha,time,blockPlaceType,false};
                                    }
                                    else{
                                        voxels[(i*1000000)+(j*1000)+k] = {static_cast<float>((rand()%100)/100.),static_cast<float>((rand()%100)/100.),static_cast<float>((rand()%100)/100.),galpha,time,blockPlaceType,false};
                                        drawnVoxels[(i*1000000)+(j*1000)+k] = {gred,ggreen,gblue,galpha,time,blockPlaceType,false};
                                    }
                                }
                                else{                           //If rainbow, use the voxel position for its color
                                    voxels[(i*1000000)+(j*1000)+k] = voxel {i*size/20,j*size/20,k*size/20,galpha,time,blockPlaceType,false};
                                    drawnVoxels[(i*1000000)+(j*1000)+k] = voxel {i*size/20,j*size/20,k*size/20,galpha,time,blockPlaceType,false};
                                }
                                if (voxels[(i*1000000)+(j*1000)+k].blockType == 4){
                                    voxels[(i*1000000)+(j*1000)+k].fertile = drawnVoxels[(i*1000000)+(j*1000)+k].fertile = true;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        
        
        
        //If we're placing new mass
        if (spray == 0){
            int time = glutGet(GLUT_ELAPSED_TIME);
            //These three 'for' loops iterate through all voxels inside a cube surrounding the highlighted voxel
            for (int i = xloc-diameter; i < xloc+diameter+1; i++){
                for (int j = yloc-diameter; j < yloc + diameter+1; j++){
                    for (int k = zloc-diameter; k < zloc + diameter+1; k++){
                        
                        
                        //Distance is how far the current voxel is from the center voxel
                        float distance = pow(pow(yloc-j,2)+pow(xloc-i,2)+pow(zloc-k,2),.5);
                        //This says 'if the voxel's distance from center voxel == diameter' (so diameter is more a radius)
                        //and is responsible for leaving the insides of large brushes empty.  only makes shells.
                        if (distance <= diameter  && ((diameter - distance) <1.5) ){
                            if (i >= 0 && i < resolution && j >=0 && j < resolution && k >= 0 && k < resolution){
                                
                                //put it in the checklist as 'true' or 'a voxel exists here'
                                voxelCheckList[i][j][k] = true;
                                
                                
                                if (!rainbow){                  //If not rainbow, use the brush colors.
                                    //Here we actually put the voxel on the map. (do it a few lines down if rainbow mode)
                                    if (!randomness){
                                        voxels[(i*1000000)+(j*1000)+k] = {gred,ggreen,gblue,galpha,time,blockPlaceType,false};
                                        drawnVoxels[(i*1000000)+(j*1000)+k] = {gred,ggreen,gblue,galpha,time,blockPlaceType,false};
                                    }
                                    else{
                                        voxels[(i*1000000)+(j*1000)+k] = {static_cast<float>((rand()%100)/100.),static_cast<float>((rand()%100)/100.),static_cast<float>((rand()%100)/100.),galpha,time,blockPlaceType,false};
                                        drawnVoxels[(i*1000000)+(j*1000)+k] = {gred,ggreen,gblue,galpha,time,blockPlaceType,false};
                                    }
                                }
                                else{                           //If rainbow, use the voxel position for its color
                                    voxels[(i*1000000)+(j*1000)+k] = {i*size/20,j*size/20,k*size/20,galpha,time,blockPlaceType,false};
                                    drawnVoxels[(i*1000000)+(j*1000)+k] = {i*size/20,j*size/20,k*size/20,galpha,time,blockPlaceType,false};
                                }
                                
                                if (voxels[(i*1000000)+(j*1000)+k].blockType == 4){
                                    voxels[(i*1000000)+(j*1000)+k].fertile = drawnVoxels[(i*1000000)+(j*1000)+k].fertile = true;
                                }
                                
                                //This will mark the hidden voxels in voxelSecretList
                                updateVoxels(i,j,k);
                            }
                        }
                    }
                }
            }
        }
    }
    
    
    
    
    //-----------------ERASE
    //if erase is selected, erase this voxel
    if (erase){
        
        int xloc = floor((xlock/size)+.5);
        int yloc = floor((ylock/size)+.5);
        int zloc = floor((zlock/size)+.5);
        
        for (int i = xloc-diameter; i < xloc+diameter+1; i++){
            for (int j = yloc-diameter; j < yloc + diameter+1; j++){
                for (int k = zloc-diameter; k < zloc + diameter+1; k++){
                    float distance = pow(pow(yloc-j,2)+pow(xloc-i,2)+pow(zloc-k,2),.5);
                    if (distance <= diameter){
                        if (i >= 0 && i < resolution && j >=0 && j < resolution && k >= 0 && k < resolution){
                            
                            voxels.erase((i*1000000)+(j*1000)+k);                   //this part erases voxels
                            drawnVoxels.erase((i*1000000)+(j*1000)+k);
                            voxelCheckList[i][j][k] = false;            //remove it from the voxel checklist array
                            
                            
                            
                            //Marks that secret list again since you're changing voxels again
                            {
                                
                                if (i+1 < resolution){
                                    voxelSecretList[i+1][j][k] = closedIn(i+1,j,k);
                                    if (!voxelSecretList[i+1][j][k]){
                                        if (voxelCheckList[i+1][j][k]){
                                            drawnVoxels.insert ( std::pair<int,voxel> ( ((i+1)*1000000)+(j*1000)+k , voxels[((i+1)*1000000)+(j*1000)+k] ) );
                                        }
                                    }
                                }
                                
                                if (i > 0){
                                    voxelSecretList[i-1][j][k] = closedIn(i-1,j,k);
                                    if (!voxelSecretList[i-1][j][k]){
                                        if (voxelCheckList[i-1][j][k]){
                                            drawnVoxels.insert ( std::pair<int,voxel> ( ((i-1)*1000000)+(j*1000)+k , voxels[((i-1)*1000000)+(j*1000)+k] ) );
                                        }
                                    }
                                }
                                
                                if (j+1 < resolution){
                                    voxelSecretList[i][j+1][k] = closedIn(i,j+1,k);
                                    if (!voxelSecretList[i][j+1][k]){
                                        if (voxelCheckList[i][j+1][k]){
                                            drawnVoxels.insert ( std::pair<int,voxel> ( (i*1000000)+((j+1)*1000)+k , voxels[(i*1000000)+((j+1)*1000)+k] ) );
                                        }
                                    }
                                }
                                
                                if (j > 0){
                                    voxelSecretList[i][j-1][k] = closedIn(i,j-1,k);
                                    if (!voxelSecretList[i][j-1][k]){
                                        if (voxelCheckList[i][j-1][k]){
                                            drawnVoxels.insert ( std::pair<int,voxel> ( (i*1000000)+((j-1)*1000)+k , voxels[(i*1000000)+((j-1)*1000)+k] ) );
                                        }
                                    }
                                }
                                
                                if (k+1 < resolution){
                                    voxelSecretList[i][j][k+1] = closedIn(i,j,k+1);
                                    if (!voxelSecretList[i][j][k+1]){
                                        if (voxelCheckList[i][j][k+1]){
                                            drawnVoxels.insert ( std::pair<int,voxel> ( (i*1000000)+(j*1000)+k+1 , voxels[(i*1000000)+(j*1000)+k+1] ) );
                                        }
                                    }
                                }
                                
                                if (k > 0){
                                    voxelSecretList[i][j][k-1] = closedIn(i,j,k-1);
                                    if (!voxelSecretList[i][j][k-1]){
                                        if (voxelCheckList[i][j][k-1]){
                                            drawnVoxels.insert ( std::pair<int,voxel> ( (i*1000000)+(j*1000)+k-1 , voxels[(i*1000000)+(j*1000)+k-1] ) );
                                        }
                                    }
                                }
                            }
                            
                        }
                    }
                }
            }
        }
    }
    
    
    
    
    //-----------------ACTUALLY DRAW THE VOXELS
    //Draw voxels in the voxelmap
    std::unordered_map<int, voxel>::iterator iter;
    for (iter = voxels.begin(); iter != voxels.end(); ++iter) {
        
        int xind = (iter->first)/1000000;
        int yind = ((iter->first)/1000)%1000;
        int zind = (iter->first)%1000;
        
        
        glPushMatrix();
        glTranslatef(xind*size,yind*size,zind*size);
        
        
        
        
        
        
        //Voxels of block type 0 are to be drawn normally
        if (iter->second.blockType == 0){
            drawVoxel(iter->second.red,iter->second.green,iter->second.blue,iter->second.alpha, xind, yind, zind, false);
        }
        
        //Blocktype 1 voxels are black but pulse with their colors
        else if (iter->second.blockType == 1){
            
            //Change that divisor at the end there to slow the pulses. 447 syncs it to the awesome song "think about the way". lower = faster
            float time = (glutGet(GLUT_ELAPSED_TIME)-(iter->second.start))/1000.*.5;
            
            float splitSeconds = 1-(time - int(time));
            
            drawVoxel(iter->second.red*pow(splitSeconds,.8), iter->second.green*pow(splitSeconds,5), iter->second.blue*pow(splitSeconds,1.1),iter->second.alpha, xind, yind, zind, false);
            
            //            drawVoxel(iter->second.red*((.5*cos(6.14*splitSeconds))+.5), iter->second.green*((.5*cos(6.14*splitSeconds))+.5), iter->second.blue*((.5*cos(6.14*splitSeconds))+.5),iter->second.alpha);
            
            
            //                        drawVoxel(time/20, time/10, splitSeconds,grid[r][c][l].alpha+(15/diam)-(time*.004));
            
            //                        if (grid[r][c][l].alpha+(15/diam)-(time*.004) <= 0){
            //                            grid[r][c][l].red = grid[r][c][l].blue = grid[r][c][l].green = grid[r][c][l].alpha = grid[r][c][l].start = 0;
            //                        }
        }
        
        //Blocktype 2 voxels are white but pulse with their colors
        else if (iter->second.blockType == 2){
            float time = (glutGet(GLUT_ELAPSED_TIME)-(iter->second.start))/ 1000.*.5;
            
            float splitSeconds = (time - int(time));
            
            drawVoxel(iter->second.red+(pow(splitSeconds,.6)*(1-iter->second.red)), iter->second.green+(pow(splitSeconds,.7)*(1-iter->second.green)), iter->second.blue+(pow(splitSeconds,.3)*(1-iter->second.blue)),iter->second.alpha, xind, yind, zind, false);
        }
        
        //Blocktype 3 voxels flicker like neon lights
        else if (iter->second.blockType == 3){
            //            drawVoxel(iter->second.red+flicker,iter->second.green+flicker,iter->second.blue+flicker,iter->second.alpha, xind, yind, zind, false);
            float time = (glutGet(GLUT_ELAPSED_TIME)-(iter->second.start))/1000.*.5;
            
            float splitSeconds = 1-(time - int(time));
            drawVoxel(iter->second.red*(splitSeconds+1)+flicker,iter->second.green*(splitSeconds+1)+flicker,iter->second.blue*(splitSeconds+1)+flicker,iter->second.alpha, xind, yind, zind, false);
        }
        
        
        else if (iter->second.blockType == 4){
            float time = (glutGet(GLUT_ELAPSED_TIME)-(iter->second.start));
            if (iter->second.fertile){
                //Draw it, and if snakeSpeed ms have passed, reproduce!  Then set its fertility to false.
                drawVoxel(iter->second.red,iter->second.green,iter->second.blue,iter->second.alpha, xind, yind, zind, false);
                if (time >= snakeSpeed){
                    iter->second.fertile = false;
                    toReproduce.push(iter->first);
                }
            }
            else{
                //if 20*snakeSpeed ms have passed, it should be deleted from the list and not drawn. otherwise, draw it.
                if (time >= snakeSpeed*2.5){
                    toDie.push(iter->first);
                }
                else{
                    drawVoxel(iter->second.red,iter->second.green,iter->second.blue,iter->second.alpha, xind, yind, zind, false);
                }
            }
        }
        
        
        glPopMatrix();
    }
    
    //Graph lines
    {
        //Floor
        glPushMatrix();
        glTranslatef(0,0,20);
        glRotatef(-90,1,0,0);
        drawGrid();
        glPopMatrix();
        
        //Ceiling
        glPushMatrix();
        glRotatef(90,1,0,0);
        glTranslatef(0,0,-20);
        drawGrid();
        glPopMatrix();
        
        //Front
        drawGrid();
        
        //Back
        glPushMatrix();
        glTranslatef(20,0,20);
        glRotatef(180,0,1,0);
        drawGrid();
        glPopMatrix();
        
        //Left
        glPushMatrix();
        glTranslatef(0,0,20);
        glRotatef(90,0,1,0);
        drawGrid();
        glPopMatrix();
        
        //Right
        glPushMatrix();
        glTranslatef(20,0,0);
        glRotatef(-90,0,1,0);
        drawGrid();
        glPopMatrix();
    }
    
    
    while (!toDie.empty()){
        int index = toDie.front();
        drawnVoxels.erase(index);
        voxels.erase(index);
        voxelCheckList[index/1000000][(index/1000)%1000][index%1000] = voxelSecretList[index/1000000][(index/1000)%1000][index%1000] = false;
        toDie.pop();
    }
    
    int guessCount;
    while (!toReproduce.empty()){
        int index = toReproduce.front();
        guessCount = 0;
        
        while (guessCount < 7){
            int dir = rand()%6;
            //left
            if (dir == 2){
                if ((index/1000000)-1 >= 0){
                    if (!voxelCheckList[(index/1000000)-1][(index/1000)%1000][index%1000]){
                        //Update the voxel's existence
                        voxelCheckList[(index/1000000)-1][(index/1000)%1000][index%1000] = true;
                        
                        //place it
                        voxels[index-1000000] = voxels[index];
                        voxels[index-1000000].start += snakeSpeed;
                        voxels[index-1000000].fertile = true;
                        drawnVoxels[index-1000000] = voxels[index-1000000];
                        
                        updateVoxels((index/1000000)-1,(index/1000)%1000,index%1000);
                        break;
                    }
                    else{
                        guessCount++;
                    }
                }
            }
            //right
            if (dir == 3){
                if ((index/1000000)+1 < resolution){
                    if (!voxelCheckList[(index/1000000)+1][(index/1000)%1000][index%1000]){
                        //Update the voxel's existence
                        voxelCheckList[(index/1000000)+1][(index/1000)%1000][index%1000] = true;
                        
                        //place it
                        voxels[index+1000000] = voxels[index];
                        voxels[index+1000000].start += snakeSpeed;
                        voxels[index+1000000].fertile = true;
                        drawnVoxels[index+1000000] = voxels[index+1000000];
                        
                        updateVoxels((index/1000000)+1,(index/1000)%1000,index%1000);
                        break;
                    }
                    else{
                        guessCount++;
                    }
                }
            }
            //below
            if (dir == 0){
                if (((index/1000)%1000)-1 >= 0){
                    if (!voxelCheckList[index/1000000][((index/1000)%1000)-1][index%1000]){
                        //Update the voxel's existence
                        voxelCheckList[index/1000000][((index/1000)%1000)-1][index%1000] = true;
                        
                        //place it
                        voxels[index-1000] = voxels[index];
                        voxels[index-1000].start += snakeSpeed;
                        voxels[index-1000].fertile = true;
                        drawnVoxels[index-1000] = voxels[index-1000];
                        
                        updateVoxels(index/1000000,((index/1000)%1000)-1,index%1000);
                        break;
                    }
                    else{
                        guessCount++;
                    }
                }
            }
            //top
            if (dir == 8){
                if (((index/1000)%1000)+1 < resolution){
                    if (!voxelCheckList[index/1000000][((index/1000)%1000)+1][index%1000]){
                        //Update the voxel's existence
                        voxelCheckList[index/1000000][((index/1000)%1000)+1][index%1000] = true;
                        
                        //place it
                        voxels[index+1000] = voxels[index];
                        voxels[index+1000].start += snakeSpeed;
                        voxels[index+1000].fertile = true;
                        drawnVoxels[index+1000] = voxels[index+1000];
                        
                        updateVoxels(index/1000000,((index/1000)%1000)+1,index%1000);
                        break;
                    }
                    else{
                        guessCount++;
                    }
                }
            }
            //behind
            if (dir == 4){
                if ((index%1000)-1 >= 0){
                    if (!voxelCheckList[index/1000000][(index/1000)%1000][(index%1000)-1]){
                        //Update the voxel's existence
                        voxelCheckList[index/1000000][(index/1000)%1000][(index%1000)-1] = true;
                        
                        //place it
                        voxels[index-1] = voxels[index];
                        voxels[index-1].start += snakeSpeed;
                        voxels[index-1].fertile = true;
                        drawnVoxels[index-1] = voxels[index-1];
                        
                        updateVoxels(index/1000000,(index/1000)%1000,(index%1000)-1);
                        break;
                    }
                    else{
                        guessCount++;
                    }
                }
            }
            //front
            if (dir == 1){
                if ((index%1000)+1 < resolution){
                    if (!voxelCheckList[index/1000000][(index/1000)%1000][(index%1000)+1]){
                        //Update the voxel's existence
                        voxelCheckList[index/1000000][(index/1000)%1000][(index%1000)+1] = true;
                        
                        //place it
                        voxels[index+1] = voxels[index];
                        voxels[index+1].start += snakeSpeed;
                        voxels[index+1].fertile = true;
                        drawnVoxels[index+1] = voxels[index+1];
                        
                        updateVoxels(index/1000000,(index/1000)%1000,(index%1000)+1);
                        break;
                    }
                    else{
                        guessCount++;
                    }
                }
            }
        }
        
        toReproduce.pop();
    }
    
    
    
    
    glutSwapBuffers();
}





//When you press a key
void processNormalKeys(unsigned char key, int xx, int yy) {
    
    if (key == 27)                  //press escape to exit
        exit(0);
    
    //press 'c' to change the brush color
    if (key == 'c'){
        colorChange = true;
        colorSet(x+lx, y+ly, z+lz, gred, ggreen, gblue);
        startAng = angle;
        startPosX = x;
        startPosZ = z;
    }
    
    if (key == 'a'){                 //press 'a' to change rainbow mode
        if (rainbow == true){
            rainbow = false;
        }
        else{
            rainbow = true;
        }
    }
    
    if (key == 'b'){                 //if you press 'b', change the brush type
        blockPlaceType++;
        if (blockPlaceType == 5){
            blockPlaceType = 0;
        }
    }
    
    if (key == '['){                //if you press '['--decrease brush size
        if (spray == 0){
            diameter -=1;
            if (diameter <.7){
                diameter = .7;
            }
        }
        else{
            sprayDiameter-=1;
            if (sprayDiameter < 1){
                sprayDiameter = 1;
            }
        }
    }
    if (key == ']'){                 //if you press ']'--increase brush size
        if (spray == 0){
            diameter += 1;
        }
        else{
            sprayDiameter += 1;
        }
    }
    
    if (key == ','){                 //if you press ','--bring the brush closer
        proximity -= .2;
    }
    
    if (key == '.'){                 //if you press '.'--move the brush away
        proximity += .2;
    }
    
    if (key == 'r'){                //if you press 'r', reset the blocks
        setUp();
    }
    
    if (key == 's'){                //if you hit 's', switch paint/spray modes
        spray++;
        if (spray == 3){
            spray = 0;
        }
    }
    
    if (key == 'x'){                //if you hit 'x', make the room brighter
        roomShade += .05;
    }
    
    if (key == 'z'){                //if you hit 'z', darken the room.
        roomShade -= .05;
        if (roomShade < 0){
            roomShade = 0;
        }
    }
    
    if (key == 'f'){                //if you press 'f', fill the block
        draw = true;
        erase = false;
    }
    
    if (key == 'e'){                //if you press 'e', erase the block
        
        erase = true;
        draw = false;
    }
    
    if (key == '-'){                 //if you press '-'
        galpha -= .1;
        if (galpha < 0){
            galpha = 0;
        }
    }
    
    if (key == '='){                 //if you press '='
        galpha += .1;
        if (galpha > 1){
            galpha = 1;
        }
    }
    
    if (key == 'w'){
        if (smooth){
            smooth = false;
        }
        else{
            smooth = true;
        }
    }
    
    if (key == 'q'){
        if (randomness){
            randomness = false;
        }
        else{
            randomness = true;
        }
    }
}

//Releasing the draw, erase, or color change buttons
void releaseNormalKeys(unsigned char key, int xx, int yy){
    if (key == 'f'){
        draw = false;
    }
    
    if (key == 'e'){
        erase = false;
    }
    
    if (key == 'c'){
        colorChange = false;
    }
}


//Pressing a movement key
void pressKey(int key, int xx, int yy) {
    
    switch (key) {
        case GLUT_KEY_UP :
            if (!colorChange){
                deltaMove = 0.45; break;
            }
            else{
                deltaMove = .05; break;
            }
            
        case GLUT_KEY_DOWN :
            if (!colorChange){
                deltaMove = -0.45; break;
            }
            else{
                deltaMove = -.05; break;
            }
            
        case GLUT_KEY_LEFT :
            if (!colorChange){
                deltaSide = -0.45; break;
            }
            else{
                deltaSide = -.1; break;
            }
            
        case GLUT_KEY_RIGHT :
            if (!colorChange){
                deltaSide = 0.45; break;
            }
            else{
                deltaSide = .1; break;
            }
    }
}

//Releasing a movement key
void releaseKey(int key, int x, int y) {
    
    switch (key) {
        case GLUT_KEY_UP :
        case GLUT_KEY_DOWN : deltaMove = 0;break;
        case GLUT_KEY_LEFT :
        case GLUT_KEY_RIGHT : deltaSide = 0; break;
    }
}

//Moving the mouse
void mouseMove(int x, int y) {
    
    // this will only be true when the left button is down
    if (xOrigin >= 0) {
        
		// update xdeltaAngle
		xdeltaAngle = (x - xOrigin) * 0.00009f;
        ydeltaAngle = (y - yOrigin) * 0.00009f;
	}
}

//Clicking the mouse
void mouseButton(int button, int state, int x, int y) {
    
	// only start motion if the left button is pressed
	if (button == GLUT_LEFT_BUTTON) {
        
		// when the button is released
		if (state == GLUT_UP) {
			xOrigin = -1;
            yOrigin = -1;
            xdeltaAngle = 0;
            ydeltaAngle = 0;
		}
		else  {// state = GLUT_DOWN
			xOrigin = x;
            yOrigin = y;
		}
	}
}


//Main thing
int main(int argc, char **argv) {
    
	// init GLUT and create window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(5,5);
	glutInitWindowSize(1270,690);
	glutCreateWindow("ColorCube");
    
	// register callbacks
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);
	glutIdleFunc(renderScene);
    
	glutIgnoreKeyRepeat(1);
	glutKeyboardFunc(processNormalKeys);
    glutKeyboardUpFunc(releaseNormalKeys);
	glutSpecialFunc(pressKey);
	glutSpecialUpFunc(releaseKey);
    
	// here are the two new functions
	glutMouseFunc(mouseButton);
	glutMotionFunc(mouseMove);
    
	// OpenGL init
	glEnable(GL_DEPTH_TEST);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    setUp();
    
	// enter GLUT event processing cycle
	glutMainLoop();
    
	return 1;
}