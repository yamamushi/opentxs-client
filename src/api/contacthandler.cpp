

#ifndef __STABLE_HPP__
#include <opentxs/api/stable.hpp>
#endif

#include <opentxs/api/contacthandler.hpp>
#include <opentxs/api/DBHandler.hpp>
#include <opentxs/api/mtcomms.h>

#include <opentxs/core/OTStorage.hpp>
#include <opentxs/client/OTAPI.hpp>
#include <opentxs/client/OTAPI_Exec.hpp>
#include <opentxs/client/OpenTransactions.hpp>
#include <opentxs/core/crypto/OTASCIIArmor.hpp>
#include <opentxs/client/OTWallet.hpp>

#include <opentxs/core/Log.hpp>
//#include <QObject>
//#include <QStringList>


std::string MTNameLookupQT::GetNymName(const std::string & str_id,
                                       const std::string   p_notary_id) const
{
    // ------------------------
    opentxs::otErr << String("Attempting Name Lookup on: ") << String(str_id.c_str());

    std::string str_result = this->OTNameLookup::GetNymName(str_id, p_notary_id);

    opentxs::otErr << String("Result of Name Lookup: ") << String(str_result.c_str());

    // ------------------------
    if (str_result.empty())
    {
        int nContactID = MTContactHandler::getInstance()->FindContactIDByNymID(String::fromStdString(str_id));

        if (nContactID > 0)
        {
            String contact_name = MTContactHandler::getInstance()->GetContactName(nContactID);

            if (!contact_name.isEmpty())
                str_result = contact_name.toStdString();
            // -----------------------------------------------
            if (p_notary_id != "")
                MTContactHandler::getInstance()->NotifyOfNymServerPair(String::fromStdString(str_id),
                                                                       String::fromStdString(p_notary_id));
        }
    }
    // ------------------------
    return str_result;
}

std::string MTNameLookupQT::GetAcctName(const std::string & str_id,
                                        const std::string   p_nym_id,
                                        const std::string   p_notary_id,
                                        const std::string   p_asset_id) const
{
    std::string str_result("");
    // ------------------------
    str_result = this->OTNameLookup::GetAcctName(str_id, p_nym_id, p_notary_id, p_asset_id);
    // ------------------------
    if (str_result.empty())
    {
        int nContactID = MTContactHandler::getInstance()->FindContactIDByAcctID(String::fromStdString(str_id),
                                                                                String::fromStdString(p_nym_id),
                                                                                String::fromStdString(p_notary_id),
                                                                                String::fromStdString(p_asset_id));
        if (nContactID > 0)
        {
            String contact_name = MTContactHandler::getInstance()->GetContactName(nContactID);

            if (!contact_name.isEmpty())
                str_result = contact_name.toStdString();
        }
    }
    // ------------------------
    return str_result;
}


//virtual
std::string MTNameLookupQT::GetAddressName(const std::string & str_address) const // Used for Bitmessage addresses (etc.)
{
    std::string str_result("");
    // ------------------------
    if (!str_address.empty())
    {
        int nContactID = MTContactHandler::getInstance()->GetContactByAddress(String::fromStdString(str_address));

        if (nContactID > 0)
        {
            String contact_name = MTContactHandler::getInstance()->GetContactName(nContactID);

            if (!contact_name.isEmpty())
                str_result = contact_name.toStdString();
        }
        else
        {
            String qstrNymID = MTContactHandler::getInstance()->GetNymByAddress(String::fromStdString(str_address));

            if (!qstrNymID.isEmpty())
                str_result = this->GetNymName(qstrNymID.toStdString(), "");
        }
    }
    // ------------------------
    return str_result;
}


//static
MTContactHandler * MTContactHandler::_instance = NULL;

//static
const std::string MTContactHandler::s_key_id("mc_sqlite");


//protected
MTContactHandler::MTContactHandler() : m_Mutex(QMutex::Recursive) // allows us to call it multiple times from same thread.
{
}


//static
MTContactHandler * MTContactHandler::getInstance()
{
    if (NULL == _instance)
    {
        _instance = new MTContactHandler;
    }
    return _instance;
}



bool MTContactHandler::ContactExists(int nContactID)
{
    QMutexLocker locker(&m_Mutex);
    String str_select = String("SELECT * FROM `contact` WHERE `contact_id`=%1 LIMIT 0,1").arg(nContactID);
    int nRows = DBHandler::getInstance()->querySize(str_select);
    return (nRows > 0);
}


bool MTContactHandler::DeleteContact(int nContactID)
{
    std::lock_guard<std::mutex> locker(m_Mutex);

    String str_delete_nym     = String("DELETE FROM `nym` WHERE `contact_id`=%1").arg(nContactID);
    String str_delete_method  = String("DELETE FROM `contact_method` WHERE `contact_id`=%1").arg(nContactID);
    String str_delete_contact = String("DELETE FROM `contact` WHERE `contact_id`=%1").arg(nContactID);

    return (DBHandler::getInstance()->runQuery(str_delete_nym)     &&
            DBHandler::getInstance()->runQuery(str_delete_method)  &&
            DBHandler::getInstance()->runQuery(str_delete_contact));
}

// See if a given Contact ID is associated with a given NymID.
bool MTContactHandler::VerifyNymOnExistingContact(int nContactID, String nym_id_string)
{
    const int nFoundContactID = this->FindContactIDByNymID(nym_id_string);

    if ((nFoundContactID > 0) && (nFoundContactID == nContactID))
        return true;

    return false;
}


bool MTContactHandler::AddNymToExistingContact(int nContactID, String nym_id_string)
{
    QMutexLocker locker(&m_Mutex);

    // Todo security: Make sure we don't need to encode the NymID here to prevent a SQL injection vulnerability...

    if ((nContactID > 0) && !nym_id_string.isEmpty())
    {
        // First, see if a contact already exists for this Nym, and if so,
        // save its ID and return at the bottom.
        //
        String str_select = String("SELECT `contact_id` FROM `nym` WHERE `nym_id`='%1'").arg(nym_id_string);

        qDebug() << String("Running query: %1").arg(str_select);

        int  nRows      = DBHandler::getInstance()->querySize(str_select);
        bool bNymExists = (nRows > 0); // Whether the contact ID was good or not, the Nym itself DOES exist.
        // ----------------------------------------
        String str_insert_nym;

        if (!bNymExists)
            str_insert_nym = String("INSERT INTO `nym` "
                                     "(`nym_id`, `contact_id`) "
                                     "VALUES('%1', %2)").arg(nym_id_string).arg(nContactID);
//      else if (bHadToCreateContact)
        else
            str_insert_nym = String("UPDATE nym SET contact_id=%1 WHERE nym_id='%2'").arg(nContactID).arg(nym_id_string);

        if (!str_insert_nym.isEmpty())
        {
            qDebug() << String("Running query: %1").arg(str_insert_nym);

            return DBHandler::getInstance()->runQuery(str_insert_nym);
        }
    }

    return false;
}


// TODO:
// Function to delete a contact (I think also: leaving behind the nym/account/server records for that contact...?
// Otherwise cleaning those out. Either way, still needs to wipe the nym table's record of the contact ID that got wiped.)
// Function to return a list of servers, filtered by Nym.
// Function to return a list of accounts, filtered by Nym.
// Function to return a list of accounts, filtered by Server.
// Function to return a list of accounts, filtered by Server and Asset Type.
// Function to return a list of accounts, filtered by Nym and Server.
// Function to return a list of accounts, filtered by Nym and Asset Type.
// Function to return a list of accounts, filtered by Nym, Server, and Asset Type.
// Function to return a list of asset types, filtered by Nym (account join).
// Function to return a list of asset types, filtered by Server (account join).

// NOTE: if the caller wants a list of servers NOT filtered by Nym, then he can
// just directly use the OT API. So you'd only call THIS function if you DID
// want it filtered by Nym (that's why the nym parameter isn't optional.)
//
bool MTContactHandler::GetServers(mapIDName & theMap, String filterByNym, bool bPrependOTType/*=false*/)
{
    QMutexLocker locker(&m_Mutex);

    String str_select = String("SELECT (`notary_id`) FROM `nym_server` WHERE `nym_id`='%1'").arg(filterByNym);

    bool bFoundAny = false;
    int  nRows     = DBHandler::getInstance()->querySize(str_select);

    for(int ii=0; ii < nRows; ii++)
    {
        String notary_id = DBHandler::getInstance()->queryString(str_select, 0, ii);

        if (!notary_id.isEmpty())
        {
            String server_name = String::fromStdString(opentxs::OTAPI_Wrap::It()->GetServer_Name(notary_id.toStdString()));

            if (!server_name.isEmpty())
            {
                bFoundAny = true;

                // Some callers need "otserver|notary_id" instead of just "notary_id"
                // We prepend here, if necessary.
                //
                String qstrFinalID   = notary_id;
                String qstrFinalName = server_name;

                if (bPrependOTType)
                {
                    qstrFinalID   = String("%1|%2").arg("otserver").arg(notary_id);
                    qstrFinalName = String("%1: %2").arg(QObject::tr("OT Server")).arg(server_name);
                }
                // At this point we have the server ID *and* the server name.
                // So we can add them to our map...
                theMap.insert(qstrFinalID, qstrFinalName);
            }
        }
    }
    // ---------------------------------------------------------------------
    return bFoundAny;
}


//String create_contact = "CREATE TABLE contact(contact_id INTEGER PRIMARY KEY, contact_display_name TEXT)";
//String create_nym     = "CREATE TABLE nym(nym_id TEXT PRIMARY KEY, contact_id INTEGER, nym_display_name TEXT)";
//String create_server  = "CREATE TABLE nym_server(nym_id TEXT, notary_id TEXT, PRIMARY KEY(nym_id, notary_id))";
//String create_account = "CREATE TABLE nym_account(account_id TEXT PRIMARY KEY, notary_id TEXT, nym_id TEXT, asset_id TEXT, account_display_name TEXT)";

// ---------------------------------------------------------------------
bool MTContactHandler::GetServers(mapIDName & theMap, bool bPrependOTType/*=false*/)
{
    bool    bFoundAny    = false;
    int32_t nServerCount = opentxs::OTAPI_Wrap::It()->GetServerCount();

    for (int32_t ii = 0; ii < nServerCount; ++ii)
    {
        std::string str_notary_id   = opentxs::OTAPI_Wrap::It()->GetServer_ID(ii);
        std::string str_server_name = opentxs::OTAPI_Wrap::It()->GetServer_Name(str_notary_id);

        String qstrNotaryID   = String::fromStdString(str_notary_id);
        String qstrServerName = String::fromStdString(str_server_name);

        String qstrFinalID   = qstrNotaryID;
        String qstrFinalName = qstrServerName;

        if (bPrependOTType)
        {
            qstrFinalID   = String("%1|%2").arg("otserver").arg(qstrNotaryID);
            qstrFinalName = String("%1: %2").arg(QObject::tr("OT Server")).arg(qstrServerName);
        }

        // At this point we have the server ID *and* the server name.
        // So we can add them to our map...
        if (!qstrServerName.isEmpty())
        {
            theMap.insert(qstrFinalID, qstrFinalName);
            bFoundAny = true;
        }
    }

    return bFoundAny;
}

bool MTContactHandler::GetServers(mapIDName & theMap, int nFilterByContact, bool bPrependOTType/*=false*/)
{
    QMutexLocker locker(&m_Mutex);

    String str_select = String("SELECT (`notary_id`) "
                                 "FROM `nym_server` "
                                 "INNER JOIN `nym` "
                                 "ON nym.nym_id=nym_server.nym_id "
                                 "WHERE nym.contact_id=%1").arg(nFilterByContact);

    bool bFoundAny = false;
    int  nRows     = DBHandler::getInstance()->querySize(str_select);

    for(int ii=0; ii < nRows; ii++)
    {
        String notary_id = DBHandler::getInstance()->queryString(str_select, 0, ii);

        if (!notary_id.isEmpty())
        {
            String server_name = String::fromStdString(opentxs::OTAPI_Wrap::It()->GetServer_Name(notary_id.toStdString()));

            if (!server_name.isEmpty())
            {
                bFoundAny = true;

                // Some callers need "otserver|notary_id" instead of just "notary_id"
                // We prepend here, if necessary.
                //
                String qstrFinalID   = notary_id;
                String qstrFinalName = server_name;

                if (bPrependOTType)
                {
                    qstrFinalID   = String("%1|%2").arg("otserver").arg(notary_id);
                    qstrFinalName = String("%1: %2").arg(QObject::tr("OT Server")).arg(server_name);
                }

                // At this point we have the server ID *and* the server name.
                // So we can add them to our map...
                theMap.insert(qstrFinalID, qstrFinalName);
            }
        }
    }
    // ---------------------------------------------------------------------
    return bFoundAny;
}

// The contact ID (unlike all the other IDs) is an int instead of a string.
// Therefore we just convert it to a string and return it in a map in the same
// format as all the others.
// (FYI.)
//
bool MTContactHandler::GetContacts(mapIDName & theMap)
{
    std::lock_guard<std::mutex> locker(m_Mutex);

    String str_select = String("SELECT * FROM contact");

    bool bFoundAny = false;
    int  nRows     = DBHandler::getInstance()->querySize(str_select);

    for(int ii=0; ii < nRows; ii++)
    {
        int     contact_id   = DBHandler::getInstance()->queryInt   (str_select, 0, ii);
        String contact_name = DBHandler::getInstance()->queryString(str_select, 1, ii);

        if (contact_id > 0)
        {
            bFoundAny = true;

            String str_contact_id;
            str_contact_id = String("%1").arg(contact_id);

            if (!contact_name.isEmpty())
            {
//              qDebug() << String("About to decode name: %1").arg(contact_name);

                //Decode base64.
                contact_name = Decode(contact_name);
            }
            // --------------------------------------------------
            // At this point we have the contact ID (in string form) *and* the contact name.
            // So we can add them to our map...
            theMap.insert(str_contact_id, contact_name);
        }
    }
    // ---------------------------------------------------------------------
    return bFoundAny;
}

// ---------------------------------------------------------------------

bool MTContactHandler::GetNyms(mapIDName & theMap, int nFilterByContact)
{
    std::lock_guard<std::mutex> locker(m_Mutex);

    String str_select = String("SELECT * FROM `nym` WHERE `contact_id`=%1").arg(nFilterByContact);
//  String str_select = String("SELECT * FROM `nym` WHERE `contact_id`=%1 LIMIT 0,1").arg(nFilterByContact);

    bool bFoundAny = false;
    int  nRows     = DBHandler::getInstance()->querySize(str_select);

    for (int ii=0; ii < nRows; ii++)
    {
        String nym_id   = DBHandler::getInstance()->queryString(str_select, 0, ii);
        String nym_name = DBHandler::getInstance()->queryString(str_select, 2, ii);

        if (!nym_id.isEmpty())
        {
            bFoundAny = true;

            if (!nym_name.isEmpty())
            {
//              qDebug() << String("About to decode name: %1").arg(nym_name);

                //Decode base64.
                nym_name = Decode(nym_name);
            }
            else
                nym_name = String::fromStdString(opentxs::OTAPI_Wrap::It()->GetNym_Name(nym_id.toStdString()));
            // ----------------------------
            // At this point we have the nym ID *and* the nym name.
            // So we can add them to our map...
            theMap.insert(nym_id, nym_name);
        }
    }
    // ---------------------------------------------------------------------
    return bFoundAny;
}

// ---------------------------------------------------------------------

bool MTContactHandler::GetAccounts(mapIDName & theMap, String filterByNym, String filterByServer, String filterByAsset)
{
    std::lock_guard<std::mutex> locker(&m_Mutex);
    // -------------------------
    mapIDName parameters;
    // -------------------------
    if (!filterByNym.isEmpty())
        parameters.insert("nym_id", filterByNym);
    // -------------------------
    if (!filterByServer.isEmpty())
        parameters.insert("notary_id", filterByServer);
    // -------------------------
    if (!filterByAsset.isEmpty())
        parameters.insert("asset_id", filterByAsset);
    // -------------------------
    // Construct the WHERE clause.
    //
    String strParams;

    int nIteration = 0;
    while (!parameters.empty())
    {
        nIteration++; // 1 on first iteration.
        // ----------------------------------
        mapIDName::iterator the_beginning = parameters.begin();

        if (parameters.end() == the_beginning)
            break; // Done.
        // ----------------------------------
        String strKey   = the_beginning.key();
        String strValue = the_beginning.value();
        // ----------------------------------
        if (1 == nIteration) // first iteration
        {
            strParams = String(" WHERE `%1`='%2'").arg(strKey).arg(strValue);
        }
        else // subsequent iterations.
        {
            strParams += String(" AND `%1`='%2'").arg(strKey).arg(strValue);
        }
        // ----------------------------------
        parameters.remove(strKey);
    } // while
    // ---------------------------------
    // Construct the SELECT statement and append the WHERE clause.
    //
    String str_select = String("SELECT * FROM `nym_account`");

    if (!strParams.isEmpty())
        str_select += strParams;
    // ---------------------------------
    bool bFoundAccounts = false;
    int  nRows = DBHandler::getInstance()->querySize(str_select);

    for(int ii=0; ii < nRows; ii++)
    {
        String account_id     = DBHandler::getInstance()->queryString(str_select, 0, ii);
        String account_nym_id = DBHandler::getInstance()->queryString(str_select, 2, ii);
        String display_name   = DBHandler::getInstance()->queryString(str_select, 4, ii);

        if (!display_name.isEmpty())
        {
//            qDebug() << String("About to decode name: %1").arg(display_name);

            //Decode base64.
            display_name = Decode(display_name);
        }
        //---------------------------------------------------
        if (!account_id.isEmpty()) // Account ID is present.
        {
            if (display_name.isEmpty()) // Display name isn't.
            {
                // Look up the display name for the contact who owns the Nym who owns the Acct.
                //
                if (!account_nym_id.isEmpty())
                {
                    int nContactID = this->FindContactIDByNymID(account_nym_id);

                    if (nContactID > 0)
                    {
                        display_name = this->GetContactName(nContactID);
                    }
                }
            }
            // ----------------------
            if (display_name.isEmpty()) // Display name isn't.
            {
                display_name = account_id;
            }
            // ----------------------
            bFoundAccounts = true;

            theMap.insert(account_id, display_name);
        }
    } // for
    // ---------------------------------
    return bFoundAccounts;
}

// Notice there is no "CreateContactBasedOnAcct" because you can call this first,
// and then just call FindContactIDByAcctID.
//
int MTContactHandler::CreateContactBasedOnNym(String nym_id_string, String notary_id_string/*=String("")*/)
{
    QMutexLocker locker(&m_Mutex);

    // First, see if a contact already exists for this Nym, and if so,
    // save its ID and return at the bottom.
    //
    // If no contact exists for this Nym, then create the contact and Nym.
    // (And save the contact ID, and return at the bottom.)
    //
    // Finally, do the same actions found in NotifyOfNymServerPair, only if notary_id_string
    // !isEmpty(), to make sure we record the server as well, when appropriate.
    //
    // -----------------------------------------------------------------------
    int nContactID = 0;

    // First, see if a contact already exists for this Nym, and if so,
    // save its ID and return at the bottom.
    //
    String str_select = String("SELECT `contact_id` FROM `nym` WHERE `nym_id`='%1'").arg(nym_id_string);

    qDebug() << String("Running query: %1").arg(str_select);

    int  nRows      = DBHandler::getInstance()->querySize(str_select);
    bool bNymExists = false;

    for(int ii=0; ii < nRows; ii++)
    {
        nContactID = DBHandler::getInstance()->queryInt(str_select, 0, ii);

        bNymExists = true; // Whether the contact ID was good or not, the Nym itself DOES exist.

        break; // In practice there should only be one row.
    }
    // ---------------------------------------------------------------------
    // If no contact exists for this Nym, then create the contact and Nym.
    // (And save the contact ID, and return at the bottom.)
    //
    bool bHadToCreateContact = false;
    if (!(nContactID > 0))
    {
        bHadToCreateContact = true;
        String str_insert_contact = String("INSERT INTO `contact` "
                                             "(`contact_id`) "
                                             "VALUES(NULL)");

        qDebug() << String("Running query: %1").arg(str_insert_contact);

        DBHandler::getInstance()->runQuery(str_insert_contact);
        // ----------------------------------------
        nContactID = DBHandler::getInstance()->queryInt("SELECT last_insert_rowid() from `contact`", 0, 0);
    }
    // ---------------------------------------------------------------------
    // Here we create or update the Nym...
    //
    if (nContactID > 0)
    {
        String str_insert_nym;

        // todo: add "upsert" code to consolidate to a single sql statement, if possible.

        if (!bNymExists)
            str_insert_nym = String("INSERT INTO `nym` "
                                     "(`nym_id`, `contact_id`) "
                                     "VALUES('%1', %2)").arg(nym_id_string).arg(nContactID);
//      else if (bHadToCreateContact)
        else
            str_insert_nym = String("UPDATE nym SET contact_id=%1 WHERE nym_id='%2'").arg(nContactID).arg(nym_id_string);

        if (!str_insert_nym.isEmpty())
        {
            qDebug() << String("Running query: %1").arg(str_insert_nym);

            DBHandler::getInstance()->runQuery(str_insert_nym);
        }
    }
    // ---------------------------------------------------------------------
    // Finally, do the same actions found in NotifyOfNymServerPair, only if notary_id_string
    // !isEmpty(), to make sure we record the server as well, when appropriate.
    //
    if (!notary_id_string.isEmpty())
    {
        String str_select_server = String("SELECT `notary_id` FROM `nym_server` WHERE `nym_id`='%1' AND `notary_id`='%2' LIMIT 0,1").arg(nym_id_string).arg(notary_id_string);
        int nRowsServer = DBHandler::getInstance()->querySize(str_select_server);

        if (0 == nRowsServer) // It wasn't already there. (Add it.)
        {
            String str_insert_server = String("INSERT INTO `nym_server` "
                                                "(`nym_id`, `notary_id`) "
                                                "VALUES('%1', '%2')").arg(nym_id_string).arg(notary_id_string);

            qDebug() << String("Running query: %1").arg(str_insert_server);

            DBHandler::getInstance()->runQuery(str_insert_server);
        }
    }
    // ---------------------------------------------------------------------
    return nContactID;
}

/*
int MTContactHandler::FindOrCreateContactByNym(String nym_id_string)
{
    int nContactID = this->FindContactIDByNymID(nym_id_string);

    if (nContactID > 0)
        return nContactID; // It already exists.
    // ----------------------------------------------
    "INSERT INTO `address_book` (`id`, `nym_id`, `nym_display_name`) VALUES(NULL, '%1', '%2')"
}
// NOTE: above might be unnecessary. See below comment.
*/

// Either you create a new Contact, or you use a pre-existing Contact.
//
// If you choose to add to a pre-existing Contact, then you have already
// selected him from a list, and you have his contact ID already. IN THAT
// CASE, you know the ContactID, and you have either a Nym, or a Nym/Server
// pair, or an Account (including nym, server, and asset) to add to that
// contact.
//
// Whereas if you choose to create a new Contact, you must choose to enter
// his name and then you get his new contact ID (the same as if you had
// selected him from a list) and then you can do the above, as normal.
//
// Therefore either way, we will know the Contact ID in advance, if we are
// DEFINITELY adding the data. And if we do NOT know the contact ID, that
// means we ONLY want to add the data in the case where it is found indirectly.
// (Otherwise we don't want to add it.)
//
// To some degree, we can figure out certain things automatically. For example,
// Let's say I have a receipt with some new account number on it. I might not
// have any record of that account in my contacts list. HOWEVER, the account
// might be owned by a Nym who IS in my contacts list, in which case I should
// be smart enough to add that account to the contact for that same Nym.
//
// Let's say the account isn't found in there at all, even indirectly. The function
// would then return a failure, which should be signal enough to the caller,
// to ask the user if he wants to add it to his address book. But that's not our
// place to do -- the caller will add it to his address book if he wishes. Until
// then we either just return a failure, or if we can find it indirectly, we add
// it to our records.

String MTContactHandler::GetValueByIDLowLevel(String str_select)
{
    QMutexLocker locker(&m_Mutex);

    int nRows = DBHandler::getInstance()->querySize(str_select);

    for(int ii=0; ii < nRows; ii++)
    {
        //Extract data
        String qstr_value = DBHandler::getInstance()->queryString(str_select, 0, ii);

        if (!qstr_value.isEmpty())
        {
            //Decode base64.
            qstr_value = Decode(qstr_value);
        }
        //---------------------------------------------------
        return qstr_value; // In practice there should only be one row.
    }

    return ""; // Didn't find any.
}


// Warning: this call only works after OT LoadWallet is finished.
// (Because it uses keys from the wallet.)
String MTContactHandler::GetEncryptedValueByIDLowLevel(String str_select)
{
    QMutexLocker locker(&m_Mutex);

    int nRows = DBHandler::getInstance()->querySize(str_select);

    for(int ii=0; ii < nRows; ii++)
    {
        //Extract data
        String qstr_value = DBHandler::getInstance()->queryString(str_select, 0, ii);

        if (!qstr_value.isEmpty())
        {
            //Decrypt
            qstr_value = Decrypt(qstr_value);
        }
        //---------------------------------------------------
        return qstr_value; // In practice there should only be one row.
    }

    return ""; // Didn't find any.
}


// Warning: this call only works after OT LoadWallet is finished.
// (Because it uses keys from the wallet.)
String MTContactHandler::GetEncryptedValueByID(String qstrID, String column, String table, String id_name)
{
    // NOTE: No need to lock mutex, since GetEncryptedValueByIDLowLevel already locks it.
    // -----------------------------------
    // What do the parameters mean? They are for loading generic values from tables by ID.
    //
    // For example id might be 5,
    // column might be 'nym_display_name',
    // table might be 'nym'
    // and id_name might be 'nym_id'
    // For something like:
    // String("SELECT `nym_display_name` FROM `nym` WHERE `nym_id`='%1' LIMIT 0,1").arg(qstrNymID);
    // ----------------------------------
    String str_select = String("SELECT `%1` FROM `%2` WHERE `%3`='%4' LIMIT 0,1")
            .arg(column)   // "nym_display_name"
            .arg(table)    // "nym"
            .arg(id_name)  // "nym_id"
            .arg(qstrID);     // (actual Nym ID goes here as string)

    return this->GetEncryptedValueByIDLowLevel(str_select);
}

// Warning: this call only works after OT LoadWallet is finished.
// (Because it uses keys from the wallet.)
bool MTContactHandler::SetEncryptedValueByID(String qstrID, String value, String column, String table, String id_name)
{
    QMutexLocker locker(&m_Mutex);

    String encrypted_value = Encrypt(value);
    // ------------------------------------------
    String str_update = String("UPDATE `%1` SET `%2`='%3' WHERE `%4`='%5'")
            .arg(table)         // "contact"
            .arg(column)        // "contact_display_name"
            .arg(encrypted_value) // (encrypted bitmessage connect string (for example))
            .arg(id_name)       // "contact_id"
            .arg(qstrID);       // (actual contact ID goes here)

    return DBHandler::getInstance()->runQuery(str_update);
}


// Warning: this call only works after OT LoadWallet is finished.
// (Because it uses keys from the wallet.)
String MTContactHandler::GetEncryptedValueByID(int nID, String column, String table, String id_name)
{
    // NOTE: No need to lock mutex, since GetEncryptedValueByIDLowLevel already locks it.
    // -----------------------------------
    // What do the parameters mean? They are for loading generic values from tables by ID.
    //
    // For example id might be 5,
    // column might be 'contact_display_name',
    // table might be 'contact'
    // and id_name might be 'contact_id'
    // For something like:
    // String("SELECT `contact_display_name` FROM `contact` WHERE `contact_id`=%1 LIMIT 0,1").arg(nContactID);
    // ----------------------------------
    String str_select = String("SELECT `%1` FROM `%2` WHERE `%3`=%4 LIMIT 0,1")
            .arg(column)   // "contact_display_name"
            .arg(table)    // "contact"
            .arg(id_name)  // "contact_id"
            .arg(nID);     // (actual integer ID goes here)

    return this->GetEncryptedValueByIDLowLevel(str_select);
}


// Warning: this call only works after OT LoadWallet is finished.
// (Because it uses keys from the wallet.)
bool MTContactHandler::SetEncryptedValueByID(int nID, String value, String column, String table, String id_name)
{
    QMutexLocker locker(&m_Mutex);

    String encrypted_value = Encrypt(value);
    // ------------------------------------------
    String str_update = String("UPDATE `%1` SET `%2`='%3' WHERE `%4`=%5")
            .arg(table)         // "contact"
            .arg(column)        // "contact_display_name"
            .arg(encrypted_value) // (encrypted bitmessage connect string, say.)
            .arg(id_name)       // "contact_id"
            .arg(nID);          // (actual contact ID goes here)

    return DBHandler::getInstance()->runQuery(str_update);
}



String MTContactHandler::GetValueByID(String qstrID, String column, String table, String id_name)
{
    // NOTE: No need to lock mutex, since GetValueByIDLowLevel already locks it.
    // -----------------------------------
    // What do the parameters mean? They are for loading generic values from tables by ID.
    //
    // For example id might be 5,
    // column might be 'nym_display_name',
    // table might be 'nym'
    // and id_name might be 'nym_id'
    // For something like:
    // String("SELECT `nym_display_name` FROM `nym` WHERE `nym_id`='%1' LIMIT 0,1").arg(qstrNymID);
    // ----------------------------------
    String str_select = String("SELECT `%1` FROM `%2` WHERE `%3`='%4' LIMIT 0,1")
            .arg(column)   // "nym_display_name"
            .arg(table)    // "nym"
            .arg(id_name)  // "nym_id"
            .arg(qstrID);     // (actual Nym ID goes here as string)

    return this->GetValueByIDLowLevel(str_select);
}


bool MTContactHandler::SetValueByID(String qstrID, String value, String column, String table, String id_name)
{
    QMutexLocker locker(&m_Mutex);

    String encoded_value = Encode(value);
    // ------------------------------------------
    String str_update = String("UPDATE `%1` SET `%2`='%3' WHERE `%4`='%5'")
            .arg(table)         // "contact"
            .arg(column)        // "contact_display_name"
            .arg(encoded_value) // (base64-encoded display name)
            .arg(id_name)       // "contact_id"
            .arg(qstrID);       // (actual contact ID goes here)

    return DBHandler::getInstance()->runQuery(str_update);
}


String MTContactHandler::GetValueByID(int nID, String column, String table, String id_name)
{
    // NOTE: No need to lock mutex, since GetValueByIDLowLevel already locks it.
    // -----------------------------------
    // What do the parameters mean? They are for loading generic values from tables by ID.
    //
    // For example id might be 5,
    // column might be 'contact_display_name',
    // table might be 'contact'
    // and id_name might be 'contact_id'
    // For something like:
    // String("SELECT `contact_display_name` FROM `contact` WHERE `contact_id`=%1 LIMIT 0,1").arg(nContactID);
    // ----------------------------------
    String str_select = String("SELECT `%1` FROM `%2` WHERE `%3`=%4 LIMIT 0,1")
            .arg(column)   // "contact_display_name"
            .arg(table)    // "contact"
            .arg(id_name)  // "contact_id"
            .arg(nID);     // (actual integer ID goes here)

    return this->GetValueByIDLowLevel(str_select);
}


bool MTContactHandler::SetValueByID(int nID, String value, String column, String table, String id_name)
{
    QMutexLocker locker(&m_Mutex);

    String encoded_value = Encode(value);
    // ------------------------------------------
    String str_update = String("UPDATE `%1` SET `%2`='%3' WHERE `%4`=%5")
            .arg(table)         // "contact"
            .arg(column)        // "contact_display_name"
            .arg(encoded_value) // (base64-encoded display name)
            .arg(id_name)       // "contact_id"
            .arg(nID);          // (actual contact ID goes here)

    return DBHandler::getInstance()->runQuery(str_update);
}


String MTContactHandler::GetContactName(int nContactID)
{
    return this->GetValueByID(nContactID, "contact_display_name", "contact", "contact_id");
}

bool MTContactHandler::SetContactName(int nContactID, String contact_name_string)
{
    return this->SetValueByID(nContactID, contact_name_string, "contact_display_name", "contact", "contact_id");
}

// ---------------------------------------------------

//static
String MTContactHandler::Encrypt(String plaintext)
{
    String encrypted_value("");

    if (!plaintext.isEmpty())
    {
        opentxs::OTWallet * pWallet = opentxs::OTAPI_Wrap::OTAPI()->GetWallet("MTContactHandler::Encrypt"); // This logs and ASSERTs already.

        if (NULL != pWallet)
        {
            opentxs::String strOutput, strPlaintext(plaintext.toStdString().c_str());

            if (pWallet->Encrypt_ByKeyID(s_key_id, strPlaintext, strOutput))
            {
                std::string str_temp(strOutput.Get());
                encrypted_value = String::fromStdString(str_temp);
            }

        }
    }

    return encrypted_value;
}

//static
String MTContactHandler::Decrypt(String ciphertext)
{
//  qDebug() << String("Decrypting ciphertext: %1").arg(ciphertext);

    String decrypted_value("");

    if (!ciphertext.isEmpty())
    {
        opentxs::OTWallet * pWallet = opentxs::OTAPI_Wrap::OTAPI()->GetWallet("MTContactHandler::Decrypt"); // This logs and ASSERTs already.

        if (NULL != pWallet)
        {
            opentxs::String strOutput, strCiphertext(ciphertext.toStdString().c_str());

            if (pWallet->Decrypt_ByKeyID(s_key_id, strCiphertext, strOutput))
            {
                std::string str_temp(strOutput.Get());
                decrypted_value = String::fromStdString(str_temp);
            }
 
        }
    }

    return decrypted_value;
}

// ---------------------------------------------------

//static
String MTContactHandler::Encode(String plaintext)
{
    String encoded_value("");

    if (!plaintext.isEmpty())
    {
        // Encode base64.
        opentxs::String        strValue(plaintext.toStdString());
        opentxs::OTASCIIArmor    ascValue;
        ascValue.SetString(strValue, false); //bLineBreaks=true by default
        encoded_value = String(ascValue.Get());
    }

    return encoded_value;
}

//static
String MTContactHandler::Decode(String encoded)
{
    String decoded_value("");

    if (!encoded.isEmpty())
    {
        // Decode base64.
        opentxs::OTASCIIArmor ascValue;
        ascValue.Set(encoded.toStdString().c_str());
        opentxs::String strValue;
        ascValue.GetString(strValue, false); //bLineBreaks=true by default
        decoded_value = String(strValue.Get());
    }

    return decoded_value;
}

bool MTContactHandler::MethodTypeFoundOnNym(String method_type, String filterByNym)
{
    QMutexLocker locker(&m_Mutex);

    int     nReturnValue = 0;
    String encoded_type = Encode(method_type);

    if (!encoded_type.isEmpty())
    {
        String str_select = String("SELECT * "                // Select all rows...
                                     "FROM `nym_method` "       // ...from the nym_method table...
                                     "INNER JOIN `msg_method` " // ...where it matches the 'msg_method' table...
                                     "ON nym_method.method_id=msg_method.method_id " // ...on the method_id column. (So we only see methods that are attached to a nym.)
                                     "WHERE msg_method.method_type='%1' AND nym_method.nym_id='%2' LIMIT 0,1").
                             arg(encoded_type).arg(filterByNym); // ...filtered by a method_type of 'encoded_type' function parameter. (And NymID.)

        nReturnValue = DBHandler::getInstance()->querySize(str_select);
    }

    return (nReturnValue > 0);
}

bool MTContactHandler::MethodTypeFoundOnContact(String method_type, int nFilterByContact)
{
    QMutexLocker locker(&m_Mutex);

    int     nReturnValue = 0;
    String encoded_type = Encode(method_type);

    if (!encoded_type.isEmpty())
    {
        String str_select = String("SELECT * "                // Select all rows...
                                     "FROM `contact_method` "   // ...from the contact_method table...
                                     "WHERE method_type='%1' AND contact_id=%2 LIMIT 0,1").
                arg(encoded_type).arg(nFilterByContact); // ...filtered by a method_type of 'encoded_type' function parameter.

        nReturnValue = DBHandler::getInstance()->querySize(str_select);
    }

    return (nReturnValue > 0);
}

// = "CREATE TABLE nym_method(nym_id TEXT, method_id INTEGER, address TEXT, PRIMARY KEY(nym_id, method_id, address))";
//
bool MTContactHandler::MethodTypeFoundOnAnyNym(String method_type)
{
    QMutexLocker locker(&m_Mutex);

    int     nReturnValue = 0;
    String encoded_type = Encode(method_type);

    if (!encoded_type.isEmpty())
    {
        String str_select = String("SELECT * "                // Select all rows...
                                     "FROM `nym_method` "       // ...from the nym_method table...
                                     "INNER JOIN `msg_method` " // ...where it matches the 'msg_method' table...
                                     "ON nym_method.method_id=msg_method.method_id " // ...on the method_id column. (So we only see methods that are attached to a nym.)
                                     "WHERE msg_method.method_type='%1' LIMIT 0,1").arg(encoded_type); // ...filtered by a method_type of 'encoded_type' function parameter.

        nReturnValue = DBHandler::getInstance()->querySize(str_select);
    }

    return (nReturnValue > 0);
}

// = "CREATE TABLE contact_method(contact_id INTEGER, method_type TEXT, address TEXT, PRIMARY KEY(contact_id, method_id, address))";
//
bool MTContactHandler::MethodTypeFoundOnAnyContact(String method_type)
{
    QMutexLocker locker(&m_Mutex);

    int     nReturnValue = 0;
    String encoded_type = Encode(method_type);

    if (!encoded_type.isEmpty())
    {
        String str_select = String("SELECT * "                // Select all rows...
                                     "FROM `contact_method` "   // ...from the contact_method table...
                                     "WHERE method_type='%1' LIMIT 0,1").arg(encoded_type); // ...filtered by a method_type of 'encoded_type' function parameter.

        nReturnValue = DBHandler::getInstance()->querySize(str_select);
    }

    return (nReturnValue > 0);
}


bool MTContactHandler::MethodExists(int nMethodID)
{
    QMutexLocker locker(&m_Mutex);
    String str_select = String("SELECT * FROM `msg_method` WHERE `method_id`=%1 LIMIT 0,1").arg(nMethodID);
    int nRows = DBHandler::getInstance()->querySize(str_select);
    return (nRows > 0);
}

/*
    static bool      add(const std::string type, const std::string commstring);
    static bool      get(const std::string type, mapOfNetModules & mapOutput);
    static bool      types(std::vector<std::string> & vecTypes);
    static MTComms * it ();
*/

bool MTContactHandler::MethodTypeExists(String method_type)
{
    const std::string str_method_type = method_type.toStdString();
    mapOfNetModules   mapOutput;

    if (MTComms::get(str_method_type, mapOutput))
        return true;

    return false;
}


//String create_nym_method = "CREATE TABLE nym_method(nym_id TEXT, method_id INTEGER, address TEXT, PRIMARY KEY(nym_id, method_id, address))";

bool MTContactHandler::AddMsgAddressToNym(String nym_id, int nMethodID, String address)
{
    QMutexLocker locker(&m_Mutex);

    String encoded_address = Encode(address);

    String str_select = String("SELECT `method_id` FROM `nym_method` "
                                 "WHERE `nym_id`='%1' AND `method_id`=%2 AND `address`='%3' LIMIT 0,1").
            arg(nym_id).arg(nMethodID).arg(encoded_address);

    int nRows = DBHandler::getInstance()->querySize(str_select);

    if (0 == nRows) // It wasn't already there. (Add it.)
    {
        String str_insert = String("INSERT INTO `nym_method` "
                                     "(`nym_id`, `method_id`, `address`) "
                                     "VALUES('%1', %2, '%3')").arg(nym_id).arg(nMethodID).arg(encoded_address);
        return DBHandler::getInstance()->runQuery(str_insert);
    }

    return false;
}

bool MTContactHandler::RemoveMsgAddressFromNym(String nym_id, int nMethodID, String address)
{
    QMutexLocker locker(&m_Mutex);

    String encoded_address = Encode(address);

    String str_delete = String("DELETE FROM `nym_method` "
                                 "WHERE `nym_id`='%1' AND `method_id`=%2 AND `address`='%3'")
            .arg(nym_id).arg(nMethodID).arg(encoded_address);

    return (DBHandler::getInstance()->runQuery(str_delete));
}

//String create_contact_method  = "CREATE TABLE contact_method(contact_id INTEGER, method_type TEXT, address TEXT, PRIMARY KEY(contact_id, method_id, address))";

bool MTContactHandler::AddMsgAddressToContact(int nContactID, String qstrMethodType, String address)
{
    QMutexLocker locker(&m_Mutex);

    String encoded_type    = Encode(qstrMethodType);
    String encoded_address = Encode(address);

    // See if qstrMethodType and address are already there for nContactID.
    // Like, does Contact #5 already have "bitmessage" and "lkjsdfkjdffd" in the table?
    // (If so, no need to re-add them.)
    //
    String str_select = String("SELECT `method_type` FROM `contact_method` "
                                 "WHERE `contact_id`=%1 AND `method_type`='%2' AND `address`='%3' LIMIT 0,1").
            arg(nContactID).arg(encoded_type).arg(encoded_address);

    int nRows = DBHandler::getInstance()->querySize(str_select);

    if (0 == nRows) // It wasn't already there. (Add it.)
    {
        String str_insert = String("INSERT INTO `contact_method` "
                                     "(`contact_id`, `method_type`, `address`) "
                                     "VALUES(%1, '%2', '%3')").arg(nContactID).arg(encoded_type).arg(encoded_address);
        return DBHandler::getInstance()->runQuery(str_insert);
    }

    return false;
}

bool MTContactHandler::RemoveMsgAddressFromContact(int nContactID, String qstrMethodType, String address)
{
    QMutexLocker locker(&m_Mutex);

    String encoded_type    = Encode(qstrMethodType);
    String encoded_address = Encode(address);

    String str_delete = String("DELETE FROM `contact_method` "
                                 "WHERE `contact_id`=%1 AND `method_type`='%2' AND `address`='%3'")
            .arg(nContactID).arg(encoded_type).arg(encoded_address);

    return (DBHandler::getInstance()->runQuery(str_delete));
}

// --------------------------------------------

String MTContactHandler::GetMethodDisplayName(int nMethodID)
{
    return this->GetValueByID(nMethodID, "method_display_name", "msg_method", "method_id");
}

bool MTContactHandler::SetMethodDisplayName(int nMethodID, String input)
{
    return this->SetValueByID(nMethodID, input, "method_display_name", "msg_method", "method_id");
}

String MTContactHandler::GetMethodType(int nMethodID)
{
    return this->GetValueByID(nMethodID, "method_type", "msg_method", "method_id");
}

bool MTContactHandler::SetMethodType(int nMethodID, String input)
{
    return this->SetValueByID(nMethodID, input, "method_type", "msg_method", "method_id");
}

String MTContactHandler::GetMethodTypeDisplay(int nMethodID)
{
    return this->GetValueByID(nMethodID, "method_type_display", "msg_method", "method_id");
}

bool MTContactHandler::SetMethodTypeDisplay(int nMethodID, String input)
{
    return this->SetValueByID(nMethodID, input, "method_type_display", "msg_method", "method_id");
}

String MTContactHandler::GetMethodConnectStr (int nMethodID)
{
    return this->GetEncryptedValueByID(nMethodID, "method_connect", "msg_method", "method_id");
}

bool MTContactHandler::SetMethodConnectStr (int nMethodID, String input)
{
    return this->SetEncryptedValueByID(nMethodID, input, "method_connect", "msg_method", "method_id");
}


String MTContactHandler::GetMethodType(String qstrAddress) // Get the method type based on the address.
{
    QMutexLocker locker(&m_Mutex);

    String returnVal("");
    String encoded_address = Encode(qstrAddress);

    String str_select = String("SELECT method_type "
                                 "FROM `msg_method` "
                                 "INNER JOIN `nym_method` "
                                 "ON nym_method.method_id=msg_method.method_id "
                                 "WHERE nym_method.address='%1' LIMIT 0,1").arg(encoded_address);

    int nRows = DBHandler::getInstance()->querySize(str_select);

    if (nRows > 0)
    {
        String qstrEncType = DBHandler::getInstance()->queryString(str_select, 0, 0);
        String qstrType    = Decode(qstrEncType);

        return qstrType;
    }
    // -------------------------------------------------------
    str_select = String("SELECT method_type "
                         "FROM `contact_method` "
                         "WHERE contact_method.address='%1' LIMIT 0,1").arg(encoded_address);

    nRows = DBHandler::getInstance()->querySize(str_select);

    if (nRows > 0)
    {
        String qstrEncType = DBHandler::getInstance()->queryString(str_select, 0, 0);
        String qstrType    = Decode(qstrEncType);

        return qstrType;
    }
    // -------------------------------------------------------
    return returnVal;
}

// --------------------------------------------
// A messaging method has a type (like "bitmessage") and a bitmessage connect string,
// such as "http://user:password@127.0.0.1:8332/", and a display name, like "Localhost".
// There should also be a type display name, such as "Bitmessage."
//
// If it was an OT server, the type would be "otserver" and the connect string would be
// a Server ID, such as "r1fUoHwJOWCuK3WBAAySjmKYqsG6G2TYIxdqY6YNuuG", and the display
// name would be: "Transactions.com" (from the server contract.)
// The type display name would be "OT Server"
//
// To pass this via mapIDName, for an OT server, the ID would be:
// "otserver|r1fUoHwJOWCuK3WBAAySjmKYqsG6G2TYIxdqY6YNuuG"
// Display name:  "OT Server: Transactions.com"
//
// Whereas for a Bitmessage address, the ID would be:
// "bitmessage|METHOD_ID"
// Display name:  "Bitmessage: Localhost"
//
// Need a Method ID, like Contact ID.
// Method should have: method ID, type string, connect string, display name, type display name.
//
// There should also be a table, associating message methods with nyms, and another table,
// associating methods with contacts.

// GetMsgMethods will return things like Bitmessage, Redis, ZMQ publisher, etc.
// If bAddServers is set to true, then it will include the OT servers on that
// list.
//

bool MTContactHandler::GetMsgMethods(mapIDName & theMap, bool bAddServers/*=false*/, String filterByType/*=""*/)
{
    bool bFoundAny = false;

    {   // This block is here so locker will expire at the bottom of the block.
        // (Since GetServers call, below this block, also locks the mutex.)
        //
        std::lock_guard<std::mutex> locker(m_Mutex);

        String str_select = String("SELECT * FROM `msg_method`");

        int nRows = DBHandler::getInstance()->querySize(str_select);
        // -----------------------------------
        for (int ii=0; ii < nRows; ii++)
        {
            //Extract data
            int     nMethodID          = DBHandler::getInstance()->queryInt   (str_select, 0, ii);
            String qstrEncDisplayName = DBHandler::getInstance()->queryString(str_select, 1, ii);
            String qstrEncType        = DBHandler::getInstance()->queryString(str_select, 2, ii);
            String qstrEncTypeDisplay = DBHandler::getInstance()->queryString(str_select, 3, ii);
//          String qstrEncConnect     = DBHandler::getInstance()->queryString(str_select, 4, ii);
            // -----------------------------------------------------
            String qstrDisplayName    = Decode(qstrEncDisplayName);
            String qstrType           = Decode(qstrEncType);
            String qstrTypeDisplay    = Decode(qstrEncTypeDisplay);
//          String qstrConnect        = Decrypt(qstrEncConnect);
            // -----------------------------------------------------
            // If a type filter is passed in, and it doesn't match the type of this record,
            // then continue.
            if (!filterByType.isEmpty() && (0 != filterByType.compare(qstrType)))
                continue;
            // -----------------------------------------------------
            // Whereas for a Bitmessage address, the ID would be:
            // "bitmessage|METHOD_ID"
            // Display name:  "Bitmessage: Localhost"
            //
//            String qstrID   = filterByType.isEmpty() ? String("%1|%2").arg(qstrType).arg(nMethodID) : String("%1").arg(nMethodID);
//            String qstrName = filterByType.isEmpty() ? String("%1: %2").arg(qstrTypeDisplay).arg(qstrDisplayName) : qstrDisplayName;
            String qstrID   = String("%1").arg(nMethodID);
            String qstrName = qstrDisplayName;

            theMap.insert(qstrID, qstrName);

            bFoundAny = true;
        }
    }
    // --------------------------------------------
    bool bGotServers = false;

    if (bAddServers)
        bGotServers = this->GetServers(theMap, true); //bPrependOTType=false by default

    return bFoundAny || bGotServers;
}



/*
class MTComms
{
    static bool      add(const std::string type, const std::string commstring);
    static bool      get(const std::string type, mapOfNetModules & mapOutput);
    static bool      types(std::vector<std::string> & vecTypes);
    static MTComms * it ();
};
*/

bool MTContactHandler::GetMsgMethodTypes(mapIDName & theMap, bool bAddServers/*=false*/)
{
    bool bFoundAny = false;

    mapOfCommTypes mapTypes;

    if (MTComms::types(mapTypes))
    {
        for(mapOfCommTypes::iterator it = mapTypes.begin(); it != mapTypes.end(); it++)
        {
            std::string strType = it->first;
            std::string strName = it->second;

            String    qstrType = String::fromStdString(strType);
            String    qstrName = String::fromStdString(strName);

            if ((qstrType.size() > 0) && (qstrName.size() > 0))
            {
                theMap.insert(qstrType, qstrName);
                bFoundAny = true;
            }
        }
    }
    // --------------------------------------------
    bool bGotServers = false;

    if (bAddServers)
        bGotServers = this->GetServers(theMap, true); //bPrependOTType=false by default

    return bFoundAny || bGotServers;
}

// --------------------------------------------

bool MTContactHandler::GetMethodsAndAddrByNym(mapIDName & theMap, String filterByNym)
{
    bool bFoundAny = false;
    // ----------------------
    mapIDName mapMethods;

    bool bGotMethods = this->GetMsgMethodsByNym(mapMethods, filterByNym, false, String(""));

    if (bGotMethods)
    {
        // Loop through mapMethods and for each methodID, call GetAddressesByNym.
        // Then add those addresses, each iteration, to theMap.
        //
        for (mapIDName::iterator ii = mapMethods.begin(); ii != mapMethods.end(); ++ii)
        {
            String qstrID        = ii.key();
            int nFilterByMethodID = 0;

            StringList stringlist = qstrID.split("|");

            if (stringlist.size() >= 2) // Should always be 2...
            {
//              String qstrType     = stringlist.at(0);
                String qstrMethodID = stringlist.at(1);
                const int  nMethodID = qstrMethodID.isEmpty() ? 0 : qstrMethodID.toInt();
                // --------------------------------------
                if (nMethodID > 0)
                {
                    mapIDName mapAddresses;

                    if (this->GetMethodsAndAddrByNym(mapAddresses, filterByNym, nMethodID))
                    {
                        for (mapIDName::iterator jj = mapAddresses.begin(); jj != mapAddresses.end(); ++jj)
                        {
                            String qstrAddress = jj.key();

                            if (!theMap.contains(qstrAddress))
                            {
                                bFoundAny = true;
                                theMap.insertMulti(qstrAddress, jj.value());
                            }
                        } // for
                    } // if GetAddressesByNym
                } // if nMethodID > 0
            }
        } // for
     } // if bGotMethods
    // ----------------------
    return bFoundAny;
}


bool MTContactHandler::GetAddressesByNym(mapIDName & theMap, String filterByNym, String filterByType)
{
    bool bFoundAny = false;
    // ----------------------
    mapIDName mapMethods;

    bool bGotMethods = this->GetMsgMethodsByNym(mapMethods, filterByNym, false, filterByType);

    if (bGotMethods)
    {
        // Loop through mapMethods and for each methodID, call GetAddressesByNym.
        // Then add those addresses, each iteration, to theMap.
        //
        for (mapIDName::iterator ii = mapMethods.begin(); ii != mapMethods.end(); ++ii)
        {
            String qstrMethodID  = ii.key();
            int nFilterByMethodID = qstrMethodID.toInt();

            if (nFilterByMethodID > 0)
            {
                mapIDName mapAddresses;

                if (this->GetAddressesByNym(mapAddresses, filterByNym, nFilterByMethodID))
                {
                    for (mapIDName::iterator jj = mapAddresses.begin(); jj != mapAddresses.end(); ++jj)
                    {
                        String qstrAddress = jj.key();

                        if (!theMap.contains(qstrAddress))
                        {
                            bFoundAny = true;
                            theMap.insertMulti(qstrAddress, jj.value());
                        }
                    } // for
                } // if GetAddressesByNym
            } // if nFilterByMethodID > 0
        } // for
     } // if bGotMethods
    // ----------------------
    return bFoundAny;
}

// --------------------------------------------

int MTContactHandler::GetMethodIDByNymAndAddress(String filterByNym, String qstrAddress)
{
    // = "CREATE TABLE nym_method(nym_id TEXT, method_id INTEGER, address TEXT, PRIMARY KEY(nym_id, method_id, address))";

    int nReturn = 0;

    {   // This block is here so locker will expire at the bottom of the block.
        QMutexLocker locker(&m_Mutex);

        String str_select = String("SELECT method_id "
                                     "FROM `nym_method` "
                                     "WHERE nym_id='%1' AND address='%2'").arg(filterByNym).arg(qstrAddress);

        int nRows = DBHandler::getInstance()->querySize(str_select);
        // -----------------------------------
        for (int ii=0; ii < nRows; ii++)
        {
            nReturn = DBHandler::getInstance()->queryInt(str_select, 0, ii);
            break; // Should only be one.
            // (You might have multiple addresses for the same NymID/MethodID,
            // but you won't have multiple MethodIDs for the same address. Thus,
            // should be only one.)
        }
    }
    // --------------------------------------------
    return nReturn;
}


bool MTContactHandler::GetMethodsAndAddrByNym(mapIDName & theMap, String filterByNym, int filterByMethodID)
{
    // = "CREATE TABLE nym_method(nym_id TEXT, method_id INTEGER, address TEXT, PRIMARY KEY(nym_id, method_id, address))";

    bool bFoundAny = false;

    {   // This block is here so locker will expire at the bottom of the block.
        QMutexLocker locker(&m_Mutex);

        String str_select = String("SELECT msg_method.method_id, nym_method.address, msg_method.method_type "
                                     "FROM `nym_method` "
                                     "INNER JOIN `msg_method` "
                                     "ON nym_method.method_id=msg_method.method_id "
                                     "WHERE nym_method.nym_id='%1' AND nym_method.method_id=%2").arg(filterByNym).arg(filterByMethodID);

        int nRows = DBHandler::getInstance()->querySize(str_select);
        // -----------------------------------
        for (int ii=0; ii < nRows; ii++)
        {
            //Extract data
            int     nMethodID          = DBHandler::getInstance()->queryInt   (str_select, 0, ii);
            String qstrEncAddress     = DBHandler::getInstance()->queryString(str_select, 1, ii);
            String qstrEncType        = DBHandler::getInstance()->queryString(str_select, 2, ii);
//          String qstrEncTypeDisplay = DBHandler::getInstance()->queryString(str_select, 3, ii);
            // -----------------------------------------------------
            String qstrAddress        = Decode(qstrEncAddress);
            String qstrType           = Decode(qstrEncType);
//          String qstrTypeDisplay    = Decode(qstrEncTypeDisplay);
            // -----------------------------------------------------
            std::string strTypeDisplay = MTComms::displayName(qstrType.toStdString());
            String    qstrTypeDisplay = String::fromStdString(strTypeDisplay);

            // For a Bitmessage address, the ID would be:
            // "BITMESSAGE_ADDRESS|METHOD_ID"
            // Display name:  "Bitmessage: BITMESSAGE_ADDRESS"
            //
            String qstrID   = String("%1|%2").arg(qstrAddress).arg(nMethodID);
            String qstrName = String("%1: %2").arg(qstrTypeDisplay).arg(qstrAddress);
            // --------------------------------------------
            theMap.insertMulti(qstrID, qstrName);

            bFoundAny = true;
        }
    }
    // --------------------------------------------
    return bFoundAny;
}

bool MTContactHandler::GetAddressesByNym(mapIDName & theMap, String filterByNym, int filterByMethodID)
{
    // = "CREATE TABLE nym_method(nym_id TEXT, method_id INTEGER, address TEXT, PRIMARY KEY(nym_id, method_id, address))";

    bool bFoundAny = false;

    {   // This block is here so locker will expire at the bottom of the block.
        QMutexLocker locker(&m_Mutex);

//      String str_select = String("SELECT nym_method.address, msg_method.method_type, msg_method.method_type_display "
        String str_select = String("SELECT nym_method.address, msg_method.method_type "
                                     "FROM `nym_method` "
                                     "INNER JOIN `msg_method` "
                                     "ON nym_method.method_id=msg_method.method_id "
                                     "WHERE nym_method.nym_id='%1' AND nym_method.method_id=%2").arg(filterByNym).arg(filterByMethodID);

        int nRows = DBHandler::getInstance()->querySize(str_select);
        // -----------------------------------
        for (int ii=0; ii < nRows; ii++)
        {
            //Extract data
            String qstrEncAddress     = DBHandler::getInstance()->queryString(str_select, 0, ii);
            String qstrEncType        = DBHandler::getInstance()->queryString(str_select, 1, ii);
//          String qstrEncTypeDisplay = DBHandler::getInstance()->queryString(str_select, 2, ii);
            // -----------------------------------------------------
            String qstrAddress        = Decode(qstrEncAddress);
            String qstrType           = Decode(qstrEncType);
//          String qstrTypeDisplay    = Decode(qstrEncTypeDisplay);
            // -----------------------------------------------------
            std::string strTypeDisplay = MTComms::displayName(qstrType.toStdString());
            String    qstrTypeDisplay = String::fromStdString(strTypeDisplay);

            // For a Bitmessage address, the ID would be:
            // "BITMESSAGE_ADDRESS"
            // Display name:  "Bitmessage: BITMESSAGE_ADDRESS"
            //
            String qstrID   = String("%1").arg(qstrAddress);
            String qstrName = String("%1: %2").arg(qstrTypeDisplay).arg(qstrAddress);
            // --------------------------------------------
            theMap.insertMulti(qstrID, qstrName);

            bFoundAny = true;
        }
    }
    // --------------------------------------------
    return bFoundAny;
}

bool MTContactHandler::GetMsgMethodTypesByNym(mapIDName & theMap, String filterByNym, bool bAddServers/*=false*/)
{
    bool bFoundAny = false;

    {   // This block is here so locker will expire at the bottom of the block.
        std::lock_guard<std::mutex> locker(m_Mutex);

        String str_select = String("SELECT method_type, method_type_display "
                                     "FROM `msg_method` "
                                     "INNER JOIN `nym_method` "
                                     "ON nym_method.method_id=msg_method.method_id "
                                     "WHERE nym_method.nym_id='%1'").arg(filterByNym);

        int nRows = DBHandler::getInstance()->querySize(str_select);
        // -----------------------------------
        for (int ii=0; ii < nRows; ii++)
        {
            String qstrEncType        = DBHandler::getInstance()->queryString(str_select, 0, ii);
            String qstrEncTypeDisplay = DBHandler::getInstance()->queryString(str_select, 1, ii);
//          String qstrEncAddress     = DBHandler::getInstance()->queryString(str_select, 2, ii);
            // -----------------------------------------------------
            String qstrType           = Decode(qstrEncType);
            String qstrTypeDisplay    = Decode(qstrEncTypeDisplay);
//          String qstrAddress        = Decode(qstrEncAddress);
            // -----------------------------------------------------
            // Whereas for a Bitmessage address, the ID would be:
            // "bitmessage|METHOD_ID"
            // Display name:  "Bitmessage: Localhost"
            //
            String qstrID   = String("%1").arg(qstrType);
            String qstrName = String("%1").arg(qstrTypeDisplay);
            // --------------------------------------------
            mapIDName::iterator it = theMap.find(qstrID);

            if (theMap.end() == it) // If it's not already in there.
            {
                theMap.insertMulti(qstrID, qstrName);
                bFoundAny = true;
            }
        }
    }
    // --------------------------------------------
    bool bGotServers = false;

    if (bAddServers)
        bGotServers = this->GetServers(theMap, filterByNym, true); //bPrependOTType=false by default

    return bFoundAny || bGotServers;
}


bool MTContactHandler::GetMsgMethodsByNym(mapIDName & theMap, String filterByNym, bool bAddServers/*=false*/, String filterByType/*=""*/)
{
    bool bFoundAny = false;

    {   // This block is here so locker will expire at the bottom of the block.
        std::lock_guard<std::mutex> locker(m_Mutex);

        String qstrTypeFilter("");

        if (!filterByType.isEmpty())
        {
            String qstrEncType = Encode(filterByType);
            qstrTypeFilter = String(" AND msg_method.method_type='%1'").arg(qstrEncType);
        }

        String str_select = String("SELECT msg_method.method_id, msg_method.method_display_name, msg_method.method_type, msg_method.method_type_display "
                                     "FROM `msg_method` "
                                     "INNER JOIN `nym_method` "
                                     "ON nym_method.method_id=msg_method.method_id "
                                     "WHERE nym_method.nym_id='%1'%2").arg(filterByNym).arg(qstrTypeFilter);

        int nRows = DBHandler::getInstance()->querySize(str_select);
        // -----------------------------------
        for (int ii=0; ii < nRows; ii++)
        {
            //Extract data
            int     nMethodID          = DBHandler::getInstance()->queryInt   (str_select, 0, ii);
            String qstrEncDisplayName = DBHandler::getInstance()->queryString(str_select, 1, ii);
            String qstrEncType        = DBHandler::getInstance()->queryString(str_select, 2, ii);
            String qstrEncTypeDisplay = DBHandler::getInstance()->queryString(str_select, 3, ii);
//          String qstrEncConnect     = DBHandler::getInstance()->queryString(str_select, 4, ii);
            // -----------------------------------------------------
            String qstrDisplayName    = Decode(qstrEncDisplayName);
            String qstrType           = Decode(qstrEncType);
            String qstrTypeDisplay    = Decode(qstrEncTypeDisplay);
//          String qstrConnect        = Decrypt(qstrEncConnect);
            // -----------------------------------------------------
            // Whereas for a Bitmessage address, the ID would be:
            // "bitmessage|METHOD_ID"
            // Display name:  "Bitmessage: Localhost"
            //
            String qstrID;
            String qstrName = String("%1: %2").arg(qstrTypeDisplay).arg(qstrDisplayName);

            if (!filterByType.isEmpty())
                qstrID   = String("%1").arg(nMethodID); // Method ID only. No need to put the type if we already filtered based on type.
            else
                qstrID   = String("%1|%2").arg(qstrType).arg(nMethodID); // Here we put the type and method. Caller can split the string.
            // --------------------------------------------
            theMap.insertMulti(qstrID, qstrName);

            bFoundAny = true;
        }
    }
    // --------------------------------------------
    bool bGotServers = false;

    if (bAddServers && filterByType.isEmpty())
        bGotServers = this->GetServers(theMap, filterByNym, true); //bPrependOTType=false by default

    return bFoundAny || bGotServers;
}


String MTContactHandler::GetNymByAddress(String qstrAddress)
{
    String qstrResult("");

    {   // This block is here so locker will expire at the bottom of the block.
        std::lock_guard<std::mutex> locker(m_Mutex);

        String encoded_address = Encode(qstrAddress);

        String str_select = String("SELECT nym_id "
                                     "FROM `nym_method` "
                                     "WHERE address='%1'").arg(encoded_address);

        int nRows = DBHandler::getInstance()->querySize(str_select);
        // -----------------------------------
        for (int ii=0; ii < nRows; ++ii)
            return DBHandler::getInstance()->queryString(str_select, 0, ii);
    }

    return qstrResult;
}

//String create_nym_method
// = "CREATE TABLE nym_method(nym_id TEXT, method_id INTEGER, address TEXT, PRIMARY KEY(nym_id, method_id, address))";
//String create_contact_method
// = "CREATE TABLE contact_method(contact_id INTEGER, method_type TEXT, address TEXT, PRIMARY KEY(contact_id, method_id, address))";

int MTContactHandler::GetContactByAddress(String qstrAddress)
{
    {   // This block is here so locker will expire at the bottom of the block.
        std::lock_guard<std::mutex> locker(m_Mutex);

        String encoded_address = Encode(qstrAddress);

        String str_select = String("SELECT contact_id "
                                     "FROM `contact_method` "
                                     "WHERE address='%1'").arg(encoded_address);

        int nRows = DBHandler::getInstance()->querySize(str_select);
        // -----------------------------------
        for (int ii=0; ii < nRows; ++ii)
        {
            return DBHandler::getInstance()->queryInt(str_select, 0, ii);
        }
    }

    return 0;
}

bool MTContactHandler::GetMsgMethodTypesByContact(mapIDName & theMap, int nFilterByContact, bool bAddServers/*=false*/, String filterByType/*=""*/)
{
// = "CREATE TABLE contact_method(contact_id INTEGER, method_type TEXT, address TEXT, PRIMARY KEY(contact_id, method_id, address))";

    bool bFoundAny = false;

    {   // This block is here so locker will expire at the bottom of the block.
        std::lock_guard<std::mutex> locker(m_Mutex);

        String qstrTypeFilter("");

        if (!filterByType.isEmpty())
        {
            String qstrEncType = Encode(filterByType);
            qstrTypeFilter = String(" AND method_type='%1'").arg(qstrEncType);
        }

        String str_select = String("SELECT contact_method.method_type, contact_method.address "
                                     "FROM `contact_method` "
                                     "WHERE contact_id=%1%2").arg(nFilterByContact).arg(qstrTypeFilter);

        int nRows = DBHandler::getInstance()->querySize(str_select);
        // -----------------------------------
        for (int ii=0; ii < nRows; ii++)
        {
            //Extract data
            String qstrEncType    = DBHandler::getInstance()->queryString(str_select, 0, ii);
            String qstrEncAddress = DBHandler::getInstance()->queryString(str_select, 1, ii);
            // -----------------------------------------------------
            String qstrType       = Decode(qstrEncType);
            String qstrAddress    = Decode(qstrEncAddress);
            // -----------------------------------------------------
            std::string strTypeDisplay = MTComms::displayName(qstrType.toStdString());
            String    qstrTypeDisplay = String::fromStdString(strTypeDisplay);

            // For a Bitmessage address, the ID would be:
            // "bitmessage|BITMESSAGE_ADDRESS"
            // Display name:  "Bitmessage: BITMESSAGE_ADDRESS"
            //
            String qstrID;
            String qstrName = String("%1: %2").arg(qstrTypeDisplay).arg(qstrAddress);

            if (!filterByType.isEmpty())
                qstrID   = String("%1").arg(qstrAddress); // Address only. No need to put the type if we already filtered based on type.
            else
                qstrID   = String("%1|%2").arg(qstrType).arg(qstrAddress); // Here we put the type and address. Caller can split the string.
            // --------------------------------------------
            theMap.insertMulti(qstrID, qstrName);

            bFoundAny = true;
        }
    }
    // --------------------------------------------
    bool bGotServers = false;

    if (bAddServers && filterByType.isEmpty())
        bGotServers = this->GetServers(theMap, nFilterByContact, true); //bPrependOTType=false by default

    return bFoundAny || bGotServers;
}

// --------------------------------------------

bool MTContactHandler::GetAddressesByContact(mapIDName & theMap, int nFilterByContact, String filterByType)
{
    return this->GetMsgMethodTypesByContact(theMap, nFilterByContact, false, filterByType);
}

// --------------------------------------------

/*
String create_msg_method = "CREATE TABLE msg_method"
        " (method_id INTEGER PRIMARY KEY,"   // 1, 2, etc.
        "  method_display_name TEXT,"        // "Localhost"
        "  method_type TEXT,"                // "bitmessage"
        "  method_type_display TEXT,"        // "Bitmessage"
        "  method_connect TEXT)";            // "http://username:password@http://127.0.0.1:8332/"
*/

int  MTContactHandler::AddMsgMethod(String display_name, String type, String type_display, String connect)
{
    QMutexLocker locker(&m_Mutex);

    String encoded_display_name  = Encode(display_name);
    String encoded_type          = Encode(type);
    String encoded_type_display  = Encode(type_display);
    String encrypted_connect_str = Encrypt(connect);

    String str_insert = String("INSERT INTO `msg_method` "
                                 "(`method_id`, `method_display_name`, `method_type`, `method_type_display`, `method_connect`) "
                                 "VALUES(NULL, '%1', '%2', '%3', '%4')")
            .arg(encoded_display_name).arg(encoded_type).arg(encoded_type_display).arg(encrypted_connect_str);

    DBHandler::getInstance()->runQuery(str_insert);

    int nMethodID = DBHandler::getInstance()->queryInt("SELECT last_insert_rowid() from `msg_method`", 0, 0);

    return nMethodID;
}

// --------------------------------------------

//String create_nym_method      = "CREATE TABLE nym_method(nym_id TEXT, method_id INTEGER, address TEXT, PRIMARY KEY(nym_id, method_id, address))";
//String create_contact_method  = "CREATE TABLE contact_method(contact_id INTEGER, method_type TEXT, address TEXT, PRIMARY KEY(contact_id, method_id, address))";

bool MTContactHandler::DeleteMsgMethod(int nMethodID)
{
    QMutexLocker locker(&m_Mutex);

    String str_delete_nym     = String("DELETE FROM `nym_method` "
                                 "WHERE `method_id`=%1").arg(nMethodID);
    String str_delete_msg     = String("DELETE FROM `msg_method` "
                                 "WHERE `method_id`=%1").arg(nMethodID);

    return (DBHandler::getInstance()->runQuery(str_delete_nym)     &&
            DBHandler::getInstance()->runQuery(str_delete_msg));
}

// --------------------------------------------

// If we have a specific Nym/Server pair, we check (using this function) to see
// if this server is already in my internal list for a given NymID. Notice this
// happens whether there is a CONTACT for that Nym, or not.
// If the pairing is not found, a record is added.
//
void MTContactHandler::NotifyOfNymServerPair(String nym_id_string, String notary_id_string)
{
    std::lock_guard<std::mutex> locker(m_Mutex);

    String str_select_server = String("SELECT `notary_id` FROM `nym_server` WHERE `nym_id`='%1' AND `notary_id`='%2' LIMIT 0,1").
            arg(nym_id_string).arg(notary_id_string);
    int nRowsServer = DBHandler::getInstance()->querySize(str_select_server);

    if (0 == nRowsServer) // It wasn't already there. (Add it.)
    {
        String str_insert_server = String("INSERT INTO `nym_server` "
                                            "(`nym_id`, `notary_id`) "
                                            "VALUES('%1', '%2')").arg(nym_id_string).arg(notary_id_string);
        DBHandler::getInstance()->runQuery(str_insert_server);
    }
}


// NOTE: if an account isn't ALREADY found in my contact list, I am
// very unlikely to find it in my wallet (since it's still most likely
// someone else's account) and so I am not able to look up its associated
// nymId, NotaryID, and assetTypeIds. Therefore if I have those available
// when I call this function, then I NEED to pass them in, so that if the
// account CAN be added to some existing contact, we will have its IDs to
// add.
// Therefore this function will not bother adding an account when the Nym
// is unknown.
//
int MTContactHandler::FindContactIDByAcctID(String acct_id_string,
                                            String nym_id_string/*=String("")*/,
                                            String notary_id_string/*=String("")*/,
                                            String asset_id_string/*=String("")*/)
{
    std::lock_guard<std::mutex> locker(m_Mutex);

    int nContactID = 0;

    String str_select = String("SELECT `contact_id` "
                                 "FROM `nym` "
                                 "INNER JOIN `nym_account` "
                                 "ON nym_account.nym_id=nym.nym_id "
                                 "WHERE nym_account.account_id='%1'").arg(acct_id_string);

    int nRows = DBHandler::getInstance()->querySize(str_select);

    for(int ii=0; ii < nRows; ii++)
    {
        //Extract data
        nContactID = DBHandler::getInstance()->queryInt(str_select, 0, ii);
        break;

        // IN THIS CASE, the account record already existed for the given account ID.
        // We were able to join it to the Nym table and get the contact ID as well.
        // BUT -- what if the account record is incomplete? For example, what if
        // the server ID, or asset type ID fields were blank for that account?
        //
        // If they were in the record already, but blank in this call, we'd
        // want to discard the blank values and keep the existing record. But
        // if they were blank in the record, yet available here in this call,
        // then we'd want to UPDATE the record with the latest available values.
        // We certainly wouldn't want to just return and leave the record blank,
        // when we DID have good values that good have been set.
        //
        // UPDATE: They shouldn't ever be blank, because when the account ID is
        // available, then the server and asset type also are (so they would have
        // been added.) The only ID that could be missing would be a NymID,
        // and that wouldn't put us in this block due to the JOIN on NymID.
        // Therefore BELOW this block, we have to check for cases where the account
        // exists even when the Nym doesn't, and if the Nym ID was passed in,
        // add the new Nym, and update the account record with that Nym ID.
        // Of course in that case, we are adding a Nym who has no corresponding
        // contact record, but if he ever IS added as a contact, we will be able
        // to display his name next to his account-related receipts, even those
        // which have no Nym ID associated, since those that DID will enable us
        // to look up later the ones that DON'T.
        //
    }
    // ---------------------------------
    // The above only works if we already have a record of the account AND Nym.
    // But let's say this account is new -- no record of it, even though we
    // may already know the NYM who owns this account.
    //
    // Therefore we need to check to see if the Nym is on a contact, who owns
    // this account, and if so, we need to add this account under that Nym,
    // and return the contact ID. Although it may be in reverse -- we may
    // know the account and Nym IDs now, even though we previously had a record
    // for one or the other, and up until now had no way of connecting the two.
    //
    // However, we need to add the account one way or the other, so we'll do
    // that first, and then if the following piece applies, it can always
    // assume to do an update instead of an insert.
    // ---------------------------------
    String final_notary_id = notary_id_string;
    String final_nym_id    = nym_id_string;
    // ---------------------------------
    String str_select_acct = String("SELECT * "
                                      "FROM `nym_account` "
                                      "WHERE account_id='%1'").arg(acct_id_string);

    int nRowsAcct = DBHandler::getInstance()->querySize(str_select_acct);

    if (nRowsAcct > 0) // If the account record already existed.
    {
        // Update it IF we have values worth sticking in there.
        //
        if (!notary_id_string.isEmpty() || !asset_id_string.isEmpty() || !nym_id_string.isEmpty())
        {
      //    nym_account(account_id TEXT PRIMARY KEY, notary_id TEXT, nym_id TEXT, asset_id TEXT,
      //                account_display_name TEXT)";
            String existing_notary_id = DBHandler::getInstance()->queryString(str_select_acct, 1, 0);
            String existing_asset_id  = DBHandler::getInstance()->queryString(str_select_acct, 3, 0);
            String existing_nym_id    = DBHandler::getInstance()->queryString(str_select_acct, 2, 0);

            // Here we're just making sure we don't run an update unless we've
            // actually added some new data.
            //
            if ((existing_notary_id.isEmpty() && !notary_id_string.isEmpty()) ||
                (existing_asset_id.isEmpty()  && !asset_id_string.isEmpty())  ||
                (existing_asset_id.isEmpty()  && !asset_id_string.isEmpty()) )
            {
                        final_notary_id    = !existing_notary_id.isEmpty() ? existing_notary_id : notary_id_string;
                String final_asset_id     = !existing_asset_id.isEmpty()  ? existing_asset_id  : asset_id_string;
                        final_nym_id       = !existing_nym_id.isEmpty()    ? existing_nym_id    : nym_id_string;
                // -----------------------------------------------------------------
                String str_update_acct = String("UPDATE `nym_account` SET `notary_id`='%1',`asset_id`='%2',`nym_id`='%3' WHERE `account_id`='%4'").
                        arg(final_notary_id).arg(final_asset_id).arg(final_nym_id).arg(acct_id_string);

                DBHandler::getInstance()->runQuery(str_update_acct);
            }
        }
    }
    else // the account record didn't already exist.
    {
        // Add it then.
        //
        String str_insert_acct = String("INSERT INTO `nym_account` "
                                          "(`account_id`, `notary_id`, `nym_id`, `asset_id`) "
                                          "VALUES('%1', '%2', '%3', '%4')").arg(acct_id_string).arg(notary_id_string).arg(nym_id_string).arg(asset_id_string);
        DBHandler::getInstance()->runQuery(str_insert_acct);
    }
    // By this point, the record of this account definitely exists, though we may not have previously
    // had a record of it. (Thus below, we can assume to update, rather than insert, such a record.)
    // ---------------------------------
    // Here's where we check to see if the Nym exists for this account,
    // for any other existing contact, even though the account itself
    // may not have previously existed.
    //
    if (!final_nym_id.isEmpty())
    {
        String str_select_nym = String("SELECT `contact_id` FROM `nym` WHERE `nym_id`='%1' LIMIT 0,1").arg(final_nym_id);

        int nRowsNym = DBHandler::getInstance()->querySize(str_select_nym);

        if (nRowsNym > 0) // the nymId was found!
        {
            for(int ii=0; ii < nRowsNym; ii++)
            {
                // Found it! (If we're in this loop.) This means a Contact was found in the contact db
                // who already contained a Nym with an ID matching the NymID on this account.
                //
                nContactID = DBHandler::getInstance()->queryInt(str_select_nym, 0, ii);
                // ------------------------------------------------------------
                // So we definitely found the right contact and should return it.
                //
                break; // In practice there should only be one row.

            } // For (nym existed, even though account didn't.)
        }
        // -------------------------
        // else... nym record wasn't found, but we DO have the Nym ID as well as the account ID,
        // and we HAVE already added a record for the account. So perhaps we should add a
        // record for the Nym as well...
        //
        else
        {
            // How to "Upsert":
//          INSERT OR IGNORE INTO players (user_name, age) VALUES ("steven", 32);
//          UPDATE players SET user_name="steven", age=32 WHERE user_name="steven";

            String str_insert_nym = String("INSERT INTO `nym` "
                                             "(`nym_id`) "
                                             "VALUES('%1')").arg(final_nym_id);
            DBHandler::getInstance()->runQuery(str_insert_nym);
            // -----------------------------------------------
//          String str_update_nym = String("UPDATE `nym` "
//                                           "(`nym_id`) "
//                                           "VALUES('%1')").arg(final_nym_id);
//          DBHandler::getInstance()->runQuery(str_update_nym);
        }
        // ---------------------------------------------------------------------
        // Finally, do the same actions found in NotifyOfNymServerPair, only if final_notary_id
        // !isEmpty(), to make sure we record the server as well, when appropriate.
        //
        if (!final_notary_id.isEmpty())
        {
            String str_select_server = String("SELECT `notary_id` FROM `nym_server` WHERE `nym_id`='%1' AND `notary_id`='%2' LIMIT 0,1").arg(final_nym_id).arg(final_notary_id);
            int nRowsServer = DBHandler::getInstance()->querySize(str_select_server);

            if (0 == nRowsServer) // It wasn't already there. (Add it.)
            {
                String str_insert_server = String("INSERT INTO `nym_server` "
                                                    "(`nym_id`, `notary_id`) "
                                                    "VALUES('%1', '%2')").arg(final_nym_id).arg(final_notary_id);
                DBHandler::getInstance()->runQuery(str_insert_server);
            }
        }
    } // NymID is available (was passed in.)
    // ---------------------------------
    return nContactID;
}



int MTContactHandler::FindContactIDByNymID(String nym_id_string)
{
    QMutexLocker locker(&m_Mutex);

    String str_select = String("SELECT `contact_id` FROM `nym` WHERE `nym_id`='%1'").arg(nym_id_string);

    int nRows = DBHandler::getInstance()->querySize(str_select);

    for(int ii=0; ii < nRows; ii++)
    {
        //Extract data
        int contact_id = DBHandler::getInstance()->queryInt(str_select, 0, ii);

        return contact_id; // In practice there should only be one row.
    }

    return 0; // Didn't find anyone.
}


MTContactHandler::~MTContactHandler()
{
}





