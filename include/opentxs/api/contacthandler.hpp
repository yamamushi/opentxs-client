#ifndef CONTACTHANDLER_HPP
#define CONTACTHANDLER_HPP

#include <vector>
#include <string>

#include "WinsockWrapper.h"
#include "ExportWrapper.h"

#include <opentxs/api/OTRecordList.hpp>

// #include <core/network/Network.h>

#include <mutex>
#include <opentxs/core/String.hpp>
#include <map>


#define DEFAULT_CHEQUE_EXPIRATION 60*60*24*30 // 2592000 seconds == 30 days
typedef opentxs::String String;

class MTNameLookupQT : public opentxs::OTNameLookup
{
public:
    virtual ~MTNameLookupQT() {}

    virtual std::string GetNymName(const std::string & str_id,
                                   const std::string   p_notary_id) const;

    virtual std::string GetAcctName(const std::string & str_id,
                                    const std::string   p_nym_id,
                                    const std::string   p_notary_id,
                                    const std::string   p_asset_id) const;

    virtual std::string GetAddressName(const std::string & str_address) const; // Used for Bitmessage addresses (etc.)
};



class MTContactHandler;

typedef std::map<String, String> mapIDName; // ID, display name.

class MTContactHandler
{
private:
  static MTContactHandler * _instance;

protected:
  MTContactHandler();

  std::mutex m_Mutex;

  static const std::string s_key_id;

public:
  static MTContactHandler * getInstance();

  int FindContactIDByNymID (String nym_id_string);
  int FindContactIDByAcctID(String acct_id_string,
                            String nym_id_string=String(""),
                            String notary_id_string=String(""),
                            String asset_id_string=String(""));

  String GetContactName(int nContactID);
  bool    SetContactName(int nContactID, String contact_name_string);

  void NotifyOfNymServerPair(String nym_id_string, String notary_id_string);

  int  CreateContactBasedOnNym(String nym_id_string, String notary_id_string=String(""));

  bool AddNymToExistingContact   (int nContactID, String nym_id_string);
  bool VerifyNymOnExistingContact(int nContactID, String nym_id_string); // See if a given Contact ID is associated with a given NymID.

  bool ContactExists(int nContactID);
  bool DeleteContact(int nContactID);
  // ---------------------------------------------
  static String Encode(String plaintext);
  static String Decode(String encoded);
  // ---------------------------------------------
  static String Encrypt(String plaintext);
  static String Decrypt(String ciphertext);
  // ---------------------------------------------
  String GetValueByIDLowLevel         (String str_select);
  String GetEncryptedValueByIDLowLevel(String str_select);

  String GetValueByID(String qstrID,                 String column, String table, String id_name);
  bool    SetValueByID(String qstrID, String value,  String column, String table, String id_name);
  String GetValueByID(int     nID,                    String column, String table, String id_name);
  bool    SetValueByID(int     nID,    String value,  String column, String table, String id_name);

  String GetEncryptedValueByID(String qstrID,                 String column, String table, String id_name);
  bool    SetEncryptedValueByID(String qstrID, String value,  String column, String table, String id_name);
  String GetEncryptedValueByID(int     nID,                    String column, String table, String id_name);
  bool    SetEncryptedValueByID(int     nID,    String value,  String column, String table, String id_name);

  /*
  String create_msg_method = "CREATE TABLE msg_method"
          " (method_id INTEGER PRIMARY KEY,"   // 1, 2, etc.
          "  method_display_name TEXT,"        // "Localhost"
          "  method_type TEXT,"                // "bitmessage"
          "  method_type_display TEXT,"        // "Bitmessage"
          "  method_connect TEXT)";            // "http://username:password@http://127.0.0.1:8332/"
  */
  // --------------------------------------------
  int  AddMsgMethod(String display_name, String type, String type_display, String connect);
  bool DeleteMsgMethod(int nMethodID);

  bool MethodExists               (int     nMethodID);
  bool MethodTypeExists           (String method_type);
  bool MethodTypeFoundOnNym       (String method_type, String filterByNym);
  bool MethodTypeFoundOnContact   (String method_type, int nFilterByContact);
  bool MethodTypeFoundOnAnyNym    (String method_type);
  bool MethodTypeFoundOnAnyContact(String method_type);

//String create_nym_method
// = "CREATE TABLE nym_method(nym_id TEXT, method_id INTEGER, address TEXT, PRIMARY KEY(nym_id, method_id, address))";
//String create_contact_method
// = "CREATE TABLE contact_method(contact_id INTEGER, method_type TEXT, address TEXT, PRIMARY KEY(contact_id, method_type, address))";

  bool AddMsgAddressToNym    (String nym_id, int     nMethodID,      String address);
  bool AddMsgAddressToContact(int nContactID, String qstrMethodType, String address);

  bool RemoveMsgAddressFromNym    (String nym_id, int     nMethodID,      String address);
  bool RemoveMsgAddressFromContact(int nContactID, String qstrMethodType, String address);
  // --------------------------------------------
  String GetMethodDisplayName(int nMethodID);
  bool    SetMethodDisplayName(int nMethodID, String input);

  String GetMethodType       (int nMethodID);
  String GetMethodType       (String qstrAddress); // Get the method type based on the address.
  bool    SetMethodType       (int nMethodID, String input);

  String GetMethodTypeDisplay(int nMethodID);
  bool    SetMethodTypeDisplay(int nMethodID, String input);

  String GetMethodConnectStr (int nMethodID);
  bool    SetMethodConnectStr (int nMethodID, String input);
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
  // Whereas for a Bitmessage address associated with one of my Nyms, the ID would be:
  // "bitmessage|METHOD_ID"
  // Display name:  "Bitmessage: Localhost"
  //
  // Whereas for a Bitmessage address associated with one of my Contacts, the ID would be:
  // "bitmessage|BITMESSAGE_ID"
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
  bool GetMsgMethods             (mapIDName & theMap,                       bool bAddServers=false, String filterByType="");
  bool GetMsgMethodTypes         (mapIDName & theMap,                       bool bAddServers=false);
  bool GetMsgMethodsByNym        (mapIDName & theMap, String filterByNym,  bool bAddServers=false, String filterByType=""); // Methods.
  bool GetMsgMethodTypesByContact(mapIDName & theMap, int nFilterByContact, bool bAddServers=false, String filterByType=""); // Method Types.
  bool GetMsgMethodTypesByNym    (mapIDName & theMap, String filterByNym,  bool bAddServers=false);

  bool GetAddressesByContact     (mapIDName & theMap, int nFilterByContact, String filterByType);
  bool GetAddressesByNym         (mapIDName & theMap, String filterByNym,  String filterByType);
  bool GetAddressesByNym         (mapIDName & theMap, String filterByNym,  int filterByMethodID);

  bool GetMethodsAndAddrByNym    (mapIDName & theMap, String filterByNym,  int filterByMethodID);
  bool GetMethodsAndAddrByNym    (mapIDName & theMap, String filterByNym);

  String GetNymByAddress    (String qstrAddress);
  int     GetContactByAddress(String qstrAddress);

  int  GetMethodIDByNymAndAddress(String filterByNym, String qstrAddress);

  bool GetServers (mapIDName & theMap,                       bool bPrependOTType=false);
  bool GetServers (mapIDName & theMap, String filterByNym,  bool bPrependOTType=false);
  bool GetServers (mapIDName & theMap, int nFilterByContact, bool bPrependOTType=false);

  bool GetAccounts(mapIDName & theMap, String filterByNym, String filterByServer, String filterByAsset);

  bool GetContacts(mapIDName & theMap);
  bool GetNyms    (mapIDName & theMap, int nFilterByContact);

  public:
    ~MTContactHandler();
};

#endif // CONTACTHANDLER_HPP
