#ifndef DBHANDLER_HPP
#define DBHANDLER_HPP

#include <opentxs/api/WinsockWrapper.h>
#include <opentxs/api/ExportWrapper.h>

#include <opentxs/api/FileHandler.hpp>

#include <opentxs/core/Log.hpp>
#include <mutex>
#include <sqlite3.h>
#include <opentxs/core/String.hpp>
//#include <QVariant>

#include <memory>

/*
 * declare DB driver and filename.
 * Note that the filename path assignment is going to need to be fixed.
 */
static const String dbDriverStr = "SQLITE";
static const String dbFileNameStr = "/mc_db"; // Changed this from db/mc_db due to directory not existing.
static const String dbConnNameStr = "addressBook";

static const int PBAR_MAX_STEPS =7;


class DBHandler
{
  private:
    static DBHandler * _instance;

  protected:
    DBHandler();

    QSqlDatabase db;
    FileHandler dbFile;
    std::mutex dbMutex;
    
    bool dbConnect();
    bool isConnected();
    bool dbDisconnect();
    bool isDbExist();
    bool dbRemove();
    bool dbCreateInstance();

  public:
    static DBHandler * getInstance();

    class PreparedQuery;

    /**
     * Start a prepared query.
     * @param run The template string including placeholders.
     * @return A PreparedQuery object to be filled in.
     */
    PreparedQuery* prepareQuery(const String& run);

    bool runQuery(const String& run);

    /**
     * Run a previously prepared query.  The memory of the query is freed.
     * @param query The query, which is freed.
     * @return True in case of success.
     */
    bool runQuery(PreparedQuery* query);

    /**
     * Run a prepared query, assuming a single returned record.  This record
     * is returned, so that multiple fields can be extracted at once.
     * @param query The query, which is freed.
     * @return The returned query record.
     * @throws std::runtime_error in case of DB error or if the query returns
     *                            not exactly one record.
     */
    QSqlRecord queryOne(PreparedQuery* query);

    int querySize(String run);
    bool isNext(String run);

    int queryInt(String run, int value, int at=0);
    String queryString(String run, int value, int at=0);

    /**
     * Run a query and for each returned record, execute a callback.  The
     * callback is passed the QSqlRecord for each result.
     * @param run The query to run.
     * @param cb The callback function called.
     * @return True in case of success.
     */
    template<typename T>
      bool queryMultiple(const String& run, T cb);
    template<typename T>
      bool queryMultiple(PreparedQuery* run, T cb);

    QVariant AddressBookInsertNym(String nym_id_string, String nym_display_name_string);

    bool AddressBookUpdateNym(String nym_id_string, String nym_display_name_string, String index_id_string);
    bool AddressBookRemoveID(int ID);

    bool AddressBookUpdateDefaultNym(String ID);
    bool AddressBookUpdateDefaultAsset(String ID);
    bool AddressBookUpdateDefaultAccount(String ID);
    bool AddressBookUpdateDefaultServer(String ID);

    ~DBHandler();
};

/**
 * A prepared query.  This object is used to fill in template parameters
 * before the query is executed by a DBHandler.
 */
class DBHandler::PreparedQuery
{

  private:
    
    friend class DBHandler;

    /** The backing QSqlQuery object.  */
    QSqlQuery query;
    /** Query string for error messages.  */
    String queryStr;

    /**
     * Construct the query given the database.  This is private as it should
     * be used from within DBHandler only, in the prepareQuery method.
     * @param db QSqlDatabase used.
     * @param run SQL template string.
     */
    inline PreparedQuery (QSqlDatabase& db, const String& run)
      : query(db), queryStr(run)
    {
      query.prepare (run);
    }

    // Disable copying.
#ifndef CXX_11
    PreparedQuery ();
    PreparedQuery (const PreparedQuery&);
    PreparedQuery& operator= (const PreparedQuery&);
#endif /* !CXX_11  */

    /**
     * Execute the query.
     * @return True in case of success.
     */
    bool execute ();

  public:

    // No copying.
#ifdef CXX_11
    PreparedQuery () = delete;
    PreparedQuery (const PreparedQuery&) = delete;
    PreparedQuery& operator= (const PreparedQuery&) = delete;
#endif /* CXX_11?  */

    /**
     * Add a bound value.
     * @param name Name of the value.
     * @param val Value to set it to.
     */
    inline void
    bind (const String& name, const QVariant val)
    {
      query.bindValue (name, val);
    }

};

#include "DBHandler.tpp"

#endif // DBHANDLER_HPP
