/************************************************************
 *
 *  OTRecordList.hpp
 *
 */

/************************************************************
 -----BEGIN PGP SIGNED MESSAGE-----
 Hash: SHA1

 *                 OPEN TRANSACTIONS
 *
 *       Financial Cryptography and Digital Cash
 *       Library, Protocol, API, Server, CLI, GUI
 *
 *       -- Anonymous Numbered Accounts.
 *       -- Untraceable Digital Cash.
 *       -- Triple-Signed Receipts.
 *       -- Cheques, Vouchers, Transfers, Inboxes.
 *       -- Basket Currencies, Markets, Payment Plans.
 *       -- Signed, XML, Ricardian-style Contracts.
 *       -- Scripted smart contracts.
 *
 *  Copyright (C) 2010-2013 by "Fellow Traveler" (A pseudonym)
 *
 *  EMAIL:
 *  FellowTraveler@rayservers.net
 *
 *  BITCOIN:  1NtTPVVjDsUfDWybS4BwvHpG2pdS9RnYyQ
 *
 *  KEY FINGERPRINT (PGP Key in license file):
 *  9DD5 90EB 9292 4B48 0484  7910 0308 00ED F951 BB8E
 *
 *  OFFICIAL PROJECT WIKI(s):
 *  https://github.com/FellowTraveler/Moneychanger
 *  https://github.com/FellowTraveler/Open-Transactions/wiki
 *
 *  WEBSITE:
 *  http://www.OpenTransactions.org/
 *
 *  Components and licensing:
 *   -- Moneychanger..A Java client GUI.....LICENSE:.....GPLv3
 *   -- otlib.........A class library.......LICENSE:...LAGPLv3
 *   -- otapi.........A client API..........LICENSE:...LAGPLv3
 *   -- opentxs/ot....Command-line client...LICENSE:...LAGPLv3
 *   -- otserver......Server Application....LICENSE:....AGPLv3
 *  Github.com/FellowTraveler/Open-Transactions/wiki/Components
 *
 *  All of the above OT components were designed and written by
 *  Fellow Traveler, with the exception of Moneychanger, which
 *  was contracted out to Vicky C (bitcointrader4@gmail.com).
 *  The open-source community has since actively contributed.
 *
 *  -----------------------------------------------------
 *
 *   LICENSE:
 *   This program is free software: you can redistribute it
 *   and/or modify it under the terms of the GNU Affero
 *   General Public License as published by the Free Software
 *   Foundation, either version 3 of the License, or (at your
 *   option) any later version.
 *
 *   ADDITIONAL PERMISSION under the GNU Affero GPL version 3
 *   section 7: (This paragraph applies only to the LAGPLv3
 *   components listed above.) If you modify this Program, or
 *   any covered work, by linking or combining it with other
 *   code, such other code is not for that reason alone subject
 *   to any of the requirements of the GNU Affero GPL version 3.
 *   (==> This means if you are only using the OT API, then you
 *   don't have to open-source your code--only your changes to
 *   Open-Transactions itself must be open source. Similar to
 *   LGPLv3, except it applies to software-as-a-service, not
 *   just to distributing binaries.)
 *
 *   Extra WAIVER for OpenSSL, Lucre, and all other libraries
 *   used by Open Transactions: This program is released under
 *   the AGPL with the additional exemption that compiling,
 *   linking, and/or using OpenSSL is allowed. The same is true
 *   for any other open source libraries included in this
 *   project: complete waiver from the AGPL is hereby granted to
 *   compile, link, and/or use them with Open-Transactions,
 *   according to their own terms, as long as the rest of the
 *   Open-Transactions terms remain respected, with regard to
 *   the Open-Transactions code itself.
 *
 *   Lucre License:
 *   This code is also "dual-license", meaning that Ben Lau-
 *   rie's license must also be included and respected, since
 *   the code for Lucre is also included with Open Transactions.
 *   See Open-Transactions/src/otlib/lucre/LUCRE_LICENSE.txt
 *   The Laurie requirements are light, but if there is any
 *   problem with his license, simply remove the Lucre code.
 *   Although there are no other blind token algorithms in Open
 *   Transactions (yet. credlib is coming), the other functions
 *   will continue to operate.
 *   See Lucre on Github:  https://github.com/benlaurie/lucre
 *   -----------------------------------------------------
 *   You should have received a copy of the GNU Affero General
 *   Public License along with this program.  If not, see:
 *   http://www.gnu.org/licenses/
 *
 *   If you would like to use this software outside of the free
 *   software license, please contact FellowTraveler.
 *   (Unfortunately many will run anonymously and untraceably,
 *   so who could really stop them?)
 *
 *   DISCLAIMER:
 *   This program is distributed in the hope that it will be
 *   useful, but WITHOUT ANY WARRANTY; without even the implied
 *   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *   PURPOSE.  See the GNU Affero General Public License for
 *   more details.

 -----BEGIN PGP SIGNATURE-----
 Version: GnuPG v1.4.9 (Darwin)

 iQIcBAEBAgAGBQJRSsfJAAoJEAMIAO35UbuOQT8P/RJbka8etf7wbxdHQNAY+2cC
 vDf8J3X8VI+pwMqv6wgTVy17venMZJa4I4ikXD/MRyWV1XbTG0mBXk/7AZk7Rexk
 KTvL/U1kWiez6+8XXLye+k2JNM6v7eej8xMrqEcO0ZArh/DsLoIn1y8p8qjBI7+m
 aE7lhstDiD0z8mwRRLKFLN2IH5rAFaZZUvj5ERJaoYUKdn4c+RcQVei2YOl4T0FU
 LWND3YLoH8naqJXkaOKEN4UfJINCwxhe5Ke9wyfLWLUO7NamRkWD2T7CJ0xocnD1
 sjAzlVGNgaFDRflfIF4QhBx1Ddl6wwhJfw+d08bjqblSq8aXDkmFA7HeunSFKkdn
 oIEOEgyj+veuOMRJC5pnBJ9vV+7qRdDKQWaCKotynt4sWJDGQ9kWGWm74SsNaduN
 TPMyr9kNmGsfR69Q2Zq/FLcLX/j8ESxU+HYUB4vaARw2xEOu2xwDDv6jt0j3Vqsg
 x7rWv4S/Eh18FDNDkVRChiNoOIilLYLL6c38uMf1pnItBuxP3uhgY6COm59kVaRh
 nyGTYCDYD2TK+fI9o89F1297uDCwEJ62U0Q7iTDp5QuXCoxkPfv8/kX6lS6T3y9G
 M9mqIoLbIQ1EDntFv7/t6fUTS2+46uCrdZWbQ5RjYXdrzjij02nDmJAm2BngnZvd
 kamH0Y/n11lCvo1oQxM+
 =uSzz
 -----END PGP SIGNATURE-----
 **************************************************************/

#ifndef OPENTXS_CLIENT_OTRECORDLIST_HPP
#define OPENTXS_CLIENT_OTRECORDLIST_HPP

#include <opentxs/client/OTRecord.hpp>

#include <list>
#include <map>
#include <vector>

// For address book lookups. Your client app inherits this and provides
// addr storage/lookup through this simple interface. OTRecordList then
// calls it.
//

namespace opentxs
{

class OTNameLookup
{
public:
    EXPORT OTNameLookup()
    {
    }
    EXPORT virtual ~OTNameLookup();

    EXPORT virtual std::string GetNymName(const std::string& str_id, // NymID
                                          const std::string p_notary_id) const;

    EXPORT virtual std::string GetAcctName(
        const std::string& str_id, // AcctID
        const std::string p_nym_id, const std::string p_notary_id,
        const std::string p_instrument_definition_id) const;

    EXPORT virtual std::string GetAddressName(const std::string& str_address)
        const; // Used for Bitmessage and other special addresses.
};

/*
 // OVERLOAD THE ABOVE CLASS; make a subclass that does an address book lookup
 // however is appropriate for your client.
 //
 class OTNameLookupIPhone : public OTNameLookup
 {
 public:
    virtual std::string GetNymName(const std::string& str_id) const;
    virtual std::string GetAcctName(const std::string& str_id) const;
    virtual std::string GetAddressName(const std::string& str_id) const; //
 Used for Bitmessage and other special addresses.
 };
 */

// Client app makes an instance of its own subclass of OTNameLookup.
// Client app also makes an instance of OTLookupCaller (below.)
// Client app then gives the caller a pointer to the namelookup.
// Client app then passes the caller to OT via OT_API_Set_AddrBookCallback.
// OTRecord and OTRecordList then call the caller.
//
class OTLookupCaller
{
protected:
    OTNameLookup* _callback;

public:
    EXPORT OTLookupCaller()
        : _callback(nullptr)
    {
    }
    EXPORT ~OTLookupCaller();

    EXPORT OTNameLookup* getCallback()
    {
        return _callback;
    }
    EXPORT void delCallback();
    EXPORT void setCallback(OTNameLookup* cb);
    EXPORT bool isCallbackSet() const;

    EXPORT std::string GetNymName(const std::string& str_id, // NymID
                                  const std::string notary_id) const;

    EXPORT std::string GetAcctName(
        const std::string& str_id, // AcctID
        const std::string p_nym_id, const std::string p_notary_id,
        const std::string p_instrument_definition_id) const;

    EXPORT std::string GetAddressName(const std::string& str_address) const;
};

EXPORT bool OT_API_Set_AddrBookCallback(
    OTLookupCaller& theCaller); // OTLookupCaller must have OTNameLookup
                                // attached already.

typedef std::weak_ptr<OTRecord> weak_ptr_OTRecord;
typedef std::shared_ptr<OTRecord> shared_ptr_OTRecord;

typedef std::vector<shared_ptr_OTRecord> vec_OTRecordList;
typedef std::list<std::string> list_of_strings;
typedef std::map<std::string, std::string> map_of_strings;

class OTRecordList
{
    const OTNameLookup* m_pLookup;
    // Defaults to false. If you set it true, it will run a lot faster. (And
    // give you less data.)
    bool m_bRunFast;
    bool m_bAutoAcceptCheques; // Cheques and vouchers, NOT invoices.
    bool m_bAutoAcceptReceipts;
    bool m_bAutoAcceptTransfers;
    bool m_bAutoAcceptCash;
    static std::string s_strTextTo;   // "To: "
    static std::string s_strTextFrom; // "From: "
    list_of_strings m_servers;
    map_of_strings m_assets; // <instrument_definition_id, asset_name>
    list_of_strings m_accounts;
    list_of_strings m_nyms;
    vec_OTRecordList m_contents;
    static const std::string s_blank;
    static const std::string s_message_type;

public: // ADDRESS BOOK CALLBACK
    static bool setAddrBookCaller(OTLookupCaller& theCaller);
    static OTLookupCaller* getAddrBookCaller();

protected: // ADDRESS BOOK CALLER
    static OTLookupCaller* s_pCaller;

public:
    EXPORT OTRecordList(); // This one expects that s_pCaller is not nullptr.
    EXPORT OTRecordList(const OTNameLookup& theLookup);
    EXPORT ~OTRecordList();
    EXPORT static const char* textTo()
    {
        return s_strTextTo.c_str();
    }
    EXPORT static const char* textFrom()
    {
        return s_strTextFrom.c_str();
    }

    EXPORT static void setTextTo(std::string text)
    {
        s_strTextTo = text;
    }
    EXPORT static void setTextFrom(std::string text)
    {
        s_strTextFrom = text;
    }
    EXPORT void SetFastMode()
    {
        m_bRunFast = true;
    }
    // SETUP:

    EXPORT void SetNotaryID(std::string str_id); // Set the default server
                                                 // here.
    EXPORT void AddNotaryID(std::string str_id); // Unless you have many
                                                 // servers, then use
                                                 // this.
    EXPORT void ClearServers();                  // Also clears m_contents

    EXPORT void SetInstrumentDefinitionID(std::string str_id); // Etc.
    EXPORT void AddInstrumentDefinitionID(std::string str_id);
    EXPORT void ClearAssets(); // Also clears m_contents

    EXPORT void SetNymID(std::string str_id);
    EXPORT void AddNymID(std::string str_id);
    EXPORT void ClearNyms(); // Also clears m_contents

    EXPORT void SetAccountID(std::string str_id);
    EXPORT void AddAccountID(std::string str_id);
    EXPORT void ClearAccounts(); // Also clears m_contents

    EXPORT const list_of_strings& GetNyms() const;
    EXPORT void AcceptChequesAutomatically(bool bVal = true);
    EXPORT void AcceptReceiptsAutomatically(bool bVal = true);
    EXPORT void AcceptTransfersAutomatically(bool bVal = true);
    EXPORT void AcceptCashAutomatically(bool bVal = true);

    EXPORT bool DoesAcceptChequesAutomatically() const;
    EXPORT bool DoesAcceptReceiptsAutomatically() const;
    EXPORT bool DoesAcceptTransfersAutomatically() const;
    EXPORT bool DoesAcceptCashAutomatically() const;

    EXPORT bool PerformAutoAccept(); // Before populating, process out any items
                                     // we're supposed to accept automatically.
                                     // POPULATE:

    EXPORT bool Populate();      // Populates m_contents from OT API. Calls
                                 // ClearContents().
    EXPORT void ClearContents(); // Clears m_contents (NOT nyms, accounts,
                                 // servers, or instrument definitions.)
    EXPORT void SortRecords(); // Populate already sorts. But if you have to add
                               // some external records after Populate, then you
                               // can sort again. P.S. sorting is performed
                               // based on the "from" date.

    // Let's say you also want to add some Bitmessages. (Or any other external
    // source.) This is where you do that. Make sure to call Populate, then use
    // AddSpecialMsg a few times, then call SortRecords.
    EXPORT void AddSpecialMsg(
        const std::string& str_msg_id, // The ID of this message, from whatever
                                       // system it came from.
        bool bIsOutgoing, int32_t nMethodID,
        const std::string& str_contents, // Make sure to concatentate subject
                                         // with contents, before passing here.
        const std::string& str_address, const std::string& str_other_address,
        const std::string& str_type, const std::string& str_type_display,
        std::string str_my_nym_id = "", time64_t tDate = OT_TIME_ZERO);
    // RETRIEVE:
    //
    EXPORT int32_t size() const;
    EXPORT OTRecord GetRecord(int32_t nIndex);
    EXPORT bool RemoveRecord(int32_t nIndex);
};

} // namespace opentxs

#endif // OPENTXS_CLIENT_OTRECORDLIST_HPP
