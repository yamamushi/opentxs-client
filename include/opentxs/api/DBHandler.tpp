// Template implementation for DBHandler.h.

/**
 * Run a query and for each returned record, execute a callback.  The
 * callback is passed the QSqlRecord for each result.
 * @param run The query to run.
 * @param cb The callback function called.
 * @return True in case of success.
 */
template<typename T>
  bool
  DBHandler::queryMultiple (const String& run, T cb)
{
  std::lock_guard<std::mutex> locker(dbMutex);
  QSqlQuery query(db);

  if (!db.isOpen ())
    return false;

  const bool ok = query.exec (run);
  if (!ok)
    {
      opentxs::otErr << "runQuery: QSqlQuery::lastError: "
                << query.lastError ().text ();
      opentxs::otErr << "THE QUERY (that caused the error):" << run;
      return false;
    }

  while (query.next ())
    cb (query.record ());

  return true;
}

/**
 * Run a query and for each returned record, execute a callback.  The
 * callback is passed the QSqlRecord for each result.
 * @param run The query to run.
 * @param cb The callback function called.
 * @return True in case of success.
 */
template<typename T>
  bool
  DBHandler::queryMultiple (PreparedQuery* run, T cb)
{
#ifdef CXX_11
  std::unique_ptr<PreparedQuery> query(run);
#else /* CXX_11?  */
  std::auto_ptr<PreparedQuery> query(run);
#endif /* CXX_11?  */
  std::lock_guard<std::mutex> locker(dbMutex);

  if (!db.isOpen ())
    return false;

  const bool ok = query->execute ();
  if (!ok)
    return false;

  while (query->query.next ())
    cb (query->query.record ());

  return true;
}
