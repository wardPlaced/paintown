#ifndef _paintown_world_h
#define _paintown_world_h

class Object;
class Scene;
class Bitmap;
class Block;
class Enemy;

#include <vector>
#include <deque>
#include <map>
#include <string>

#include "util/load_exception.h"
#include "network/network.h"

namespace Script{
    class Engine;
}

struct PlayerTracker{
	double min_x;
	Object * player;
};

class World{
public:
	World();
	World( const std::vector< Object * > & players, const std::string & path, int screen_size = 320 ) throw ( LoadException );

	virtual ~World();

	void Quake( int q );

	inline const int getQuake() const {
		return quake_time;
	}

	virtual void act();
	virtual void draw( Bitmap * work );
	virtual void addObject( Object * o );

	virtual const bool finished() const;

	virtual void reloadLevel() throw( LoadException );

        virtual Script::Engine * const getEngine() const;

	/* upper left hand corner of the screen */
	virtual int getX();
	virtual int getY();

        virtual const deque<Bitmap*> & getScreenshots();

        virtual const int levelLength() const;
        virtual const Block * currentBlock() const;
        
        void addEnemy(Enemy * obj);

	virtual void doScene( int min_x, int max_x );

        virtual Object * findObject(int id);

	virtual int getMaximumZ();
	virtual int getMinimumZ();

	virtual void drawMiniMaps( bool b );
	virtual bool shouldDrawMiniMaps();

	void killAllHumans( Object * player );

	virtual void addMessage( Network::Message m, Network::Socket from = 0 );
	Network::Message createBangMessage( int x, int y, int z );

protected:

	Network::Message deleteMessage( unsigned int id );
	void loadLevel( const std::string & path ) throw( LoadException );

	void drawWorld( const PlayerTracker & tracker, Bitmap * where, const std::map< int, std::vector< Object * > > & object_z );

	virtual void deleteObjects( std::vector< Object * > * objects );

	virtual void doLogic();

	virtual const bool isPlayer( Object * o ) const;

        inline const bool shouldTakeScreenshot() const {
            return takeAScreenshot;
        }

        inline void takeScreenshot(){
            takeAScreenshot = true;
        }

        virtual void doTakeScreenshot(Bitmap * work);

protected:
	std::vector< PlayerTracker > players;
	Object * bang;
	
	std::vector< Object * > objects;

	Scene * scene;

	int quake_time;
	// int min_x;
	int screen_size;
	std::string path;

	bool draw_minimaps;
	
	Bitmap * mini_map;

        deque<Bitmap*> screenshots;
        bool takeAScreenshot;
public:
	enum{
		NOTHING,
		CREATE_CHARACTER,
		CREATE_CAT,
		CREATE_BANG,
		CREATE_ITEM,
		CREATE_PROJECTILE,
		NEXT_BLOCK,
		FINISH,
		REMOVE, /* remove object from world */
		SET_ID,
		OK,
		LOAD_LEVEL,
		DELETE_OBJ, /* remove object and delete it */
		GRAB,
		THROWN,
		IGNORE_MESSAGE,
		GAME_OVER,
                IS_PLAYER,
	};

};

#endif
