#ifndef MECACELL_WORLD_H
#define MECACELL_WORLD_H
#include <cxxabi.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <map>
#include <typeinfo>
#include <vector>
#include "connection.h"
#include "grid.hpp"
#include "integrators.hpp"
#include "model.h"
#include "modelconnection.hpp"
#include "tools.h"

using namespace std;

#undef DBG
#define DBG DEBUG(basicworld)

namespace MecaCell {
CREATE_METHOD_CHECKS(updateBehavior);

template <typename Cell, typename Integrator = Euler,
          template <class> class SpacePartition = Grid>
class BasicWorld {
 public:
	using cell_type = Cell;
	using model_type = Model;
	using integrator_type = Integrator;
	using CellCellConnectionContainer = typename Cell::CellCellConnectionContainer;
	using CellModelConnectionContainer = typename Cell::CellModelConnectionContainer;

 protected:
	float_t dt = 1.0 / 100.0;  // interval btwn updates
	size_t nbAddedCells = 0;   // used for each added cell's unique id

	// space partition hashmap for cells
	SpacePartition<Cell *> cellSpacePartition =
	    SpacePartition<Cell *>(4.5 * DEFAULT_CELL_RADIUS);

	// space partition hashmap for 3Dobj faces
	SpacePartition<std::pair<model_type *, unsigned long>> modelSpacePartition =
	    SpacePartition<std::pair<model_type *, unsigned long>>(5 * DEFAULT_CELL_RADIUS);

	// enabled collisions & connections checks
	bool cellCellCollisions = true;
	bool cellModelCollisions = true;

	// physics parameters of the world
	Vec g = Vec::zero();
	float_t viscosityCoef = 0.0003;

 public:
	int frame = 0;  // current update number
	CellCellConnectionContainer cellCellConnections;
	CellModelConnectionContainer cellModelConnections;

	// all the cells are in this container
	vector<Cell *> cells;
	// all models are stored in this map, using their name as the key
	unordered_map<string, model_type> models;

	/**********************************************
	 *                 GET & SET                  *
	 *********************************************/
	Vec getG() const { return g; }
	void setG(const Vec &v) { g = v; }
	void setDt(float_t d) { dt = d; }
	double getDt() const { return dt; }
	const decltype(cellSpacePartition) &getCellGrid() { return cellSpacePartition; }
	const decltype(cellSpacePartition) *getCellGridPtr() { return &cellSpacePartition; }
	const decltype(modelSpacePartition) &getModelGrid() { return modelSpacePartition; }
	const decltype(modelSpacePartition) *getModelGridPtr() { return &modelSpacePartition; }
	float_t getViscosityCoef() const { return viscosityCoef; }
	void setViscosityCoef(const float_t d) { viscosityCoef = d; }
	void disableCellCellCollisions() { cellCellCollisions = false; }
	int getNbUpdates() const { return frame; }
	vector<pair<Vec, Vec>> getAllVelocities() const {
		vector<pair<Vec, Vec>> res;
		res.reserve(cells.size());
		for (auto &c : cells) {
			auto f = c->getAllVelocities();
			res.insert(res.begin(), f.begin(), f.end());
		}
		return res;
	}
	vector<pair<Vec, Vec>> getAllForces() const {
		vector<pair<Vec, Vec>> res;
		res.reserve(cells.size());
		for (auto &c : cells) {
			auto f = c->getAllForces();
			res.insert(res.begin(), f.begin(), f.end());
		}
		return res;
	}
	vector<pair<Cell *, Cell *>> getConnectedCellsList() {
		unique_vector<ordered_pair<Cell *>> uniquePairs;
		for (auto &c : cells) {
			for (auto &other : c->getConnectedCells()) {
				uniquePairs.insert(make_ordered_cell_pair(c, other));
			}
		}
		vector<pair<Cell *, Cell *>> result;
		for (auto &p : uniquePairs) {
			result.push_back(make_pair(p.first, p.second));
		}
		return result;
	}
	size_t getNbOfCellCellConnections() { return getConnectedCellsList().size(); }

	static constexpr bool behaviorsEnabled =
	    has_updateBehavior_signatures<Cell, Cell *(float), Cell *(double),
	                                  Cell *(const float &),
	                                  Cell *(const double &)>::value();
	template <typename T = Cell>
	void updateBehaviors(const typename enable_if<behaviorsEnabled, T *>::type = nullptr) {
		vector<Cell *> newCells;
		for (auto &c : cells) {
			Cell *nc = c->updateBehavior(dt);
			if (nc) {
				newCells.push_back(nc);
			}
		}
		for (auto &nc : newCells) {
			addCell(nc);
		}
	}
	template <typename T = Cell>
	inline void updateBehaviors(
	    const typename enable_if<!behaviorsEnabled, T *>::type = nullptr) {}
	/**********************************************
	 *             MAIN UPDATE ROUTINE            *
	 *********************************************/

	bool nanTorques() {
		for (auto &c : cells) {
			if (isnan_v(c->getTorque())) return true;
		}
		return false;
	}
	bool nanForces() {
		for (auto &c : cells) {
			if (isnan_v(c->getForce())) return true;
		}
		return false;
	}
	bool nanPositions() {
		for (auto &c : cells) {
			if (isnan_v(c->getPosition())) return true;
		}
		return false;
	}

	void prepareCellForNextUpdate() {
		for (auto &c : cells) {
			c->updateStats();
			c->resetForces();
			c->applyExternalForces();
			c->applyExternalTorque();
			c->resetExternalForces();
			c->resetExternalTorque();
		}
	}

	void addWorldSpecificForces() {
		for (auto &c : cells) {
			c->receiveForce(-6.0 * M_PI * viscosityCoef * c->getBoundingBoxRadius() *
			                c->getVelocity());  // friction
			c->receiveForce(g * c->getMass());  // gravity
		}
	}

	void updateExistingCollisionsAndConnections() {
		Cell::updateCellCellConnections(cellCellConnections, dt);
		Cell::updateCellModelConnections(cellModelConnections, dt);
	}

	void updateCellsPositions() {
		for (auto &c : cells) c->template updatePositionsAndOrientations<Integrator>(dt);
	}

	void lookForNewCollisionsAndConnections() {
		if (cellCellCollisions)
			Cell::checkForCellCellConnections(cells, cellCellConnections, cellSpacePartition);
		if (cellModelCollisions && models.size() > 0)
			Cell::checkForCellModelConnections(cells, models, cellModelConnections,
			                                   modelSpacePartition);
	}

	void update() {
		updateModelGrid();
		prepareCellForNextUpdate();
		addWorldSpecificForces();
		updateExistingCollisionsAndConnections();
		updateCellsPositions();
		lookForNewCollisionsAndConnections();
		updateBehaviors();
		destroyDeadCells();
		++frame;
	}

	/******************************
	 *           MODELS           *
	 ******************************/
	void addModel(const string &name, const string &path) {
		models.emplace(name, path);
		models.at(name).name = name;
	}

	void removeModel(const string &name) {
		if (models.count(name)) {
			models.erase(name);
		}
		if (cellModelConnections.count(name)) {
			for (auto &c : cellModelConnections.at(name)) {
				for (auto &conn : c.second) {
					c.first->removeModelConnection(conn.second.get());
				}
			}
			cellModelConnections.erase(name);
		}
		modelSpacePartition.clear();
		for (auto &m : models) {
			insertInGrid(m.second);
		}
	}
	void insertInGrid(model_type &m) {
		for (size_t i = 0; i < m.faces.size(); ++i) {
			auto &f = m.faces[i];
			modelSpacePartition.insert({&m, i}, m.vertices[f.indices[0]],
			                           m.vertices[f.indices[1]], m.vertices[f.indices[2]]);
		}
	}
	void updateModelGrid() {
		bool modelChange = false;
		for (auto &m : models) {
			if (m.second.changedSinceLastCheck()) {
				modelChange = true;
			}
		}
		if (modelChange) {
			modelSpacePartition.clear();
			for (auto &m : models) {
				insertInGrid(m.second);
			}
		}
	}

	/******************************
	 *           MODELS           *
	 ******************************/
	void addCell(Cell *c) {
		if (c) {
			cells.push_back(c);
			c->id = nbAddedCells++;
		}
	}

	void destroyDeadCells() {
		for (auto i = cells.begin(); i != cells.end();) {
			if ((*i)->isDead()) {
				auto c = *i;
				Cell::disconnectAndDeleteAllConnections(c, cellCellConnections);
				i = cells.erase(i);
				delete c;
			} else {
				++i;
			}
		}
	}
	~BasicWorld() {
		while (!cells.empty()) delete cells.back(), cells.pop_back();
	}
};
}

#endif
