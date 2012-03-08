#include "db.hpp"
#include <stdexcept>

namespace {
   QSqlDatabase cDB;
}

using namespace DataJockey::Model;

void db::setup(
      QString type, 
      QString name, 
      QString username,
      QString password,
      int port,
      QString host) {

	cDB = QSqlDatabase::addDatabase(type);
	cDB.setDatabaseName(name);
	if(!username.isEmpty())
		cDB.setUserName(username);
	if(!password.isEmpty())
		cDB.setPassword(password);
	if(!cDB.open())
		throw std::runtime_error("cannot open database");
}

QSqlDatabase db::get() { return cDB; }
void db::close() { cDB.close(); }
