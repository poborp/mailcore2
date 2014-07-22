#include "MCMessageHeader.h"

#include "MCAddress.h"
#include "MCIterator.h"
#include "MCLibetpan.h"

#include <string.h>
#include <unistd.h>
#include <libetpan/libetpan.h>

using namespace mailcore;

static struct mailimf_address_list * lep_address_list_from_array(Array * addresses);
static struct mailimf_mailbox_list * lep_mailbox_list_from_array(Array * addresses);
static Array * lep_address_list_from_lep_addr(struct mailimf_address_list * addr_list);
static Array * lep_address_list_from_lep_mailbox(struct mailimf_mailbox_list * mb_list);

static Array * msg_id_to_string_array(clist * msgids);
static clist * msg_id_from_string_array(Array * msgids);

#define MAX_HOSTNAME 512

MessageHeader::MessageHeader()
{
    init(true, true);
}

MessageHeader::MessageHeader(MessageHeader * other)
{
    init(false, other->mMessageID == NULL);
    setMessageID(other->mMessageID);
    setReferences(other->mReferences);
    setInReplyTo(other->mInReplyTo);
    setSender(other->mSender);
    setFrom(other->mFrom);
    setTo(other->mTo);
    setCc(other->mCc);
    setBcc(other->mBcc);
    setReplyTo(other->mReplyTo);
    setSubject(other->mSubject);
    setDate(other->date());
    setReceivedDate(other->receivedDate());
    setExtraHeaders(other->mExtraHeaders);
}

void MessageHeader::init(bool generateDate, bool generateMessageID)
{
    mMessageID = NULL;
    mReferences = NULL;
    mInReplyTo = NULL;
    mSender = NULL;
    mFrom = NULL;
    mTo = NULL;
    mCc = NULL;
    mBcc = NULL;
    mReplyTo = NULL;
    mSubject = NULL;
    mDate = (time_t) -1;
    mReceivedDate = (time_t) -1;
    mExtraHeaders = NULL;
    mlcExtraHeaders = NULL;
    
    if (generateDate) {
        time_t date;
        date = time(NULL);
        setDate(date);
        setReceivedDate(date);
    }
    if (generateMessageID) {
        static String * hostname = NULL;
        static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
        
        pthread_mutex_lock(&lock);
        if (hostname == NULL) {
            char name[MAX_HOSTNAME];
            int r;

            r = gethostname(name, MAX_HOSTNAME);
            if (r < 0) {
                hostname = NULL;
            }
            else {
                hostname = new String(name);
            }
            if (hostname == NULL) {
                hostname = new String("localhost");
            }
        }
        pthread_mutex_unlock(&lock);
        
        String * messageID = new String();
        messageID->appendString(String::uuidString());
        messageID->appendUTF8Characters("@");
        messageID->appendString(hostname);
        setMessageID(messageID);
        messageID->release();
    }
}

MessageHeader::~MessageHeader()
{
    MC_SAFE_RELEASE(mMessageID);
    MC_SAFE_RELEASE(mReferences);
    MC_SAFE_RELEASE(mInReplyTo);
    MC_SAFE_RELEASE(mSender);
    MC_SAFE_RELEASE(mFrom);
    MC_SAFE_RELEASE(mTo);
    MC_SAFE_RELEASE(mCc);
    MC_SAFE_RELEASE(mBcc);
    MC_SAFE_RELEASE(mReplyTo);
    MC_SAFE_RELEASE(mSubject);
    MC_SAFE_RELEASE(mExtraHeaders);
    MC_SAFE_RELEASE(mlcExtraHeaders);
}

String * MessageHeader::description()
{
    String * result = String::string();
    result->appendUTF8Format("<%s:%p\n", className()->UTF8Characters(), this);
    if (mMessageID != NULL) {
        result->appendUTF8Format("Message-ID: %s\n", mMessageID->UTF8Characters());
    }
    if (mReferences != NULL) {
        result->appendUTF8Format("References: %s\n", mReferences->description()->UTF8Characters());
    }
    if (mInReplyTo != NULL) {
        result->appendUTF8Format("In-Reply-To: %s\n", mInReplyTo->description()->UTF8Characters());
    }
    if (mSender != NULL) {
        result->appendUTF8Format("Sender: %s\n", mSender->description()->UTF8Characters());
    }
    if (mFrom != NULL) {
        result->appendUTF8Format("From: %s\n", mFrom->description()->UTF8Characters());
    }
    if (mTo != NULL) {
        result->appendUTF8Format("To: %s\n", mTo->description()->UTF8Characters());
    }
    if (mCc != NULL) {
        result->appendUTF8Format("Cc: %s\n", mCc->description()->UTF8Characters());
    }
    if (mBcc != NULL) {
        result->appendUTF8Format("Bcc: %s\n", mBcc->description()->UTF8Characters());
    }
    if (mReplyTo != NULL) {
        result->appendUTF8Format("Reply-To: %s\n", mReplyTo->description()->UTF8Characters());
    }
    if (mSubject != NULL) {
        result->appendUTF8Format("Subject: %s\n", mSubject->UTF8Characters());
    }
    if (mExtraHeaders != NULL) {
        mc_foreachhashmapKeyAndValue(String, header, String, value, mExtraHeaders) {
            result->appendUTF8Format("%s: %s\n", header->UTF8Characters(), value->UTF8Characters());
        }
    }
    result->appendUTF8Format(">");
    
    return result;
}

Object * MessageHeader::copy()
{
    return new MessageHeader(this);
}

void MessageHeader::setMessageID(String * messageID)
{
    MC_SAFE_REPLACE_COPY(String, mMessageID, messageID);
}

String * MessageHeader::messageID()
{
    return mMessageID;
}

void MessageHeader::setReferences(Array * references)
{
    MC_SAFE_REPLACE_COPY(Array, mReferences, references);
}

Array * MessageHeader::references()
{
    return mReferences;
}

void MessageHeader::setInReplyTo(Array * inReplyTo)
{
    MC_SAFE_REPLACE_COPY(Array, mInReplyTo, inReplyTo);
}

Array * MessageHeader::inReplyTo()
{
    return mInReplyTo;
}

void MessageHeader::setDate(time_t date)
{
    mDate = date;
}

time_t MessageHeader::date()
{
    return mDate;
}

void MessageHeader::setReceivedDate(time_t date)
{
    mReceivedDate = date;
}

time_t MessageHeader::receivedDate()
{
    return mReceivedDate;
}

void MessageHeader::setSender(Address * sender)
{
    MC_SAFE_REPLACE_RETAIN(Address, mSender, sender);
}

Address * MessageHeader::sender()
{
    return mSender;
}

void MessageHeader::setFrom(Address * from)
{
    MC_SAFE_REPLACE_RETAIN(Address, mFrom, from);
}

Address * MessageHeader::from()
{
    return mFrom;
}

void MessageHeader::setTo(Array * to)
{
    MC_SAFE_REPLACE_COPY(Array, mTo, to);
}

Array * MessageHeader::to()
{
    return mTo;
}

void MessageHeader::setCc(Array * cc)
{
    MC_SAFE_REPLACE_COPY(Array, mCc, cc);
}

Array * MessageHeader::cc()
{
    return mCc;
}

void MessageHeader::setBcc(Array * bcc)
{
    MC_SAFE_REPLACE_COPY(Array, mBcc, bcc);
}

Array * MessageHeader::bcc()
{
    return mBcc;
}

void MessageHeader::setReplyTo(Array * replyTo)
{
    MC_SAFE_REPLACE_COPY(Array, mReplyTo, replyTo);
}

Array * MessageHeader::replyTo()
{
    return mReplyTo;
}

void MessageHeader::setSubject(String * subject)
{
    MC_SAFE_REPLACE_COPY(String, mSubject, subject);
}

String * MessageHeader::subject()
{
    return mSubject;
}

void MessageHeader::setUserAgent(String * userAgent)
{
    setExtraHeader(MCSTR("X-Mailer"), userAgent);
}

String * MessageHeader::userAgent()
{
    return extraHeaderValueForName(MCSTR("X-Mailer"));
}

void MessageHeader::setExtraHeaders(HashMap * headers)
{
    MC_SAFE_REPLACE_COPY(HashMap, mExtraHeaders, headers);
    MC_SAFE_RELEASE(mlcExtraHeaders);
    if (mExtraHeaders != NULL) {
        mlcExtraHeaders = new HashMap();
        mc_foreachhashmapKeyAndValue(String, key, String, value, mExtraHeaders) {
            mlcExtraHeaders->setObjectForKey(key->lowercaseString(), value);
        }
    }
}

Array * MessageHeader::allExtraHeadersNames()
{
    if (mExtraHeaders == NULL)
        return Array::array();
    return mExtraHeaders->allKeys();
}

void MessageHeader::setExtraHeader(String * name, String * object)
{
    if (mExtraHeaders == NULL) {
        mExtraHeaders = new HashMap();
    }
    if (mlcExtraHeaders == NULL) {
        mlcExtraHeaders = new HashMap();
    }
    if (object == NULL) {
        removeExtraHeader(name);
        return;
    }
    mExtraHeaders->setObjectForKey(name, object);
    mlcExtraHeaders->setObjectForKey(name->lowercaseString(), object);
}

void MessageHeader::removeExtraHeader(String * name)
{
    if (mExtraHeaders == NULL)
        return;
    mExtraHeaders->removeObjectForKey(name);
    mlcExtraHeaders->removeObjectForKey(name);
}

String * MessageHeader::extraHeaderValueForName(String * name)
{
    if (mlcExtraHeaders == NULL)
        return NULL;
    return (String *) mlcExtraHeaders->objectForKey(name->lowercaseString());
}

String * MessageHeader::extractedSubject()
{
    if (subject() == NULL)
        return NULL;
    return subject()->extractedSubject();
}

String * MessageHeader::partialExtractedSubject()
{
    if (subject() == NULL)
        return NULL;
    return subject()->extractedSubjectAndKeepBracket(true);
}

void MessageHeader::importHeadersData(Data * data)
{
    size_t cur_token;
    struct mailimf_fields * fields;
    int r;
    
    cur_token = 0;
    r = mailimf_envelope_and_optional_fields_parse(data->bytes(), data->length(), &cur_token, &fields);
    if (r != MAILIMF_NO_ERROR) {
        return;
    }
    
    importIMFFields(fields);

    mailimf_fields_free(fields);
}

void MessageHeader::importIMFFields(struct mailimf_fields * fields)
{
    struct mailimf_single_fields single_fields;
    
    mailimf_single_fields_init(&single_fields, fields);
    
    /* date */
    
    if (single_fields.fld_orig_date != NULL) {
        time_t timestamp;
        timestamp = timestampFromDate(single_fields.fld_orig_date->dt_date_time);
        setDate(timestamp);
        setReceivedDate(timestamp);
        //MCLog("%lu %lu", (unsigned long) timestamp, date());
    }
    
    /* subject */
    if (single_fields.fld_subject != NULL) {
        char * subject;
        
        subject = single_fields.fld_subject->sbj_value;
        setSubject(String::stringByDecodingMIMEHeaderValue(subject));
    }
    
    /* sender */
    if (single_fields.fld_sender != NULL) {
        struct mailimf_mailbox * mb;
        Address * address;
        
        mb = single_fields.fld_sender->snd_mb;
        if (mb != NULL) {
            address = Address::addressWithIMFMailbox(mb);
            setSender(address);
        }
    }
    
    /* from */
    if (single_fields.fld_from != NULL) {
        struct mailimf_mailbox_list * mb_list;
        Array * addresses;
        
        mb_list = single_fields.fld_from->frm_mb_list;
        addresses = lep_address_list_from_lep_mailbox(mb_list);
        if (addresses->count() > 0) {
            setFrom((Address *) (addresses->objectAtIndex(0)));
        }
    }
    
    /* replyto */
    if (single_fields.fld_reply_to != NULL) {
        struct mailimf_address_list * addr_list;
        Array * addresses;
        
        addr_list = single_fields.fld_reply_to->rt_addr_list;
        addresses = lep_address_list_from_lep_addr(addr_list);
        setReplyTo(addresses);
    }
    
    /* to */
    if (single_fields.fld_to != NULL) {
        struct mailimf_address_list * addr_list;
        Array * addresses;
        
        addr_list = single_fields.fld_to->to_addr_list;
        addresses = lep_address_list_from_lep_addr(addr_list);
        setTo(addresses);
    }
    
    /* cc */
    if (single_fields.fld_cc != NULL) {
        struct mailimf_address_list * addr_list;
        Array * addresses;
        
        addr_list = single_fields.fld_cc->cc_addr_list;
        addresses = lep_address_list_from_lep_addr(addr_list);
        setCc(addresses);
    }
    
    /* bcc */
    if (single_fields.fld_bcc != NULL) {
        struct mailimf_address_list * addr_list;
        Array * addresses;
        
        addr_list = single_fields.fld_bcc->bcc_addr_list;
        addresses = lep_address_list_from_lep_addr(addr_list);
        setBcc(addresses);
    }
    
    /* msgid */
    if (single_fields.fld_message_id != NULL) {
        char * msgid;
        String * str;
        
        msgid = single_fields.fld_message_id->mid_value;
        str = String::stringWithUTF8Characters(msgid);
        setMessageID(str);
    }
    
    /* references */
    if (single_fields.fld_references != NULL) {
        clist * msg_id_list;
        Array * msgids;
        
        msg_id_list = single_fields.fld_references->mid_list;
        msgids = msg_id_to_string_array(msg_id_list);
        setReferences(msgids);
    }
    
    /* inreplyto */
    if (single_fields.fld_in_reply_to != NULL) {
        clist * msg_id_list;
        Array * msgids;
        
        msg_id_list = single_fields.fld_in_reply_to->mid_list;
        msgids = msg_id_to_string_array(msg_id_list);
        setInReplyTo(msgids);
    }
    
    // Take care of other headers.
    for(clistiter * cur = clist_begin(fields->fld_list) ; cur != NULL ; cur = clist_next(cur)) {
        struct mailimf_field * field;

        field = (mailimf_field *)clist_content(cur);

        if (field->fld_type != MAILIMF_FIELD_OPTIONAL_FIELD) {
            continue;
        }
        
        char * fieldName;
        String * fieldNameStr;
        
        fieldName = field->fld_data.fld_optional_field->fld_name;
        fieldNameStr = String::stringWithUTF8Characters(fieldName);
        // Set only if this optional-field is not set
        if (extraHeaderValueForName(fieldNameStr) == NULL) {
            char * fieldValue;
            String * fieldValueStr;
            
            fieldValue = field->fld_data.fld_optional_field->fld_value;
            fieldValueStr = String::stringWithUTF8Characters(fieldValue);
            setExtraHeader(fieldNameStr, fieldValueStr);
        }
    }
}

#pragma mark RFC 2822 mailbox conversion

static Array * lep_address_list_from_lep_mailbox(struct mailimf_mailbox_list * mb_list)
{
    Array * result;
    clistiter * cur;
    
    result = Array::array();
    for(cur = clist_begin(mb_list->mb_list) ; cur != NULL ; cur = clist_next(cur)) {
        struct mailimf_mailbox * mb;
        Address * address;
        
        mb = (struct mailimf_mailbox *) clist_content(cur);
        address = Address::addressWithIMFMailbox(mb);
        result->addObject(address);
    }
    
    return result;
}

static Array * lep_address_list_from_lep_addr(struct mailimf_address_list * addr_list)
{
    Array * result;
    clistiter * cur;
    
    result = Array::array();
    
    if (addr_list == NULL) {
        return result;
    }
    
    if (addr_list->ad_list == NULL) {
        return result;
    }
    
    for(cur = clist_begin(addr_list->ad_list) ; cur != NULL ;
        cur = clist_next(cur)) {
        struct mailimf_address * addr;
        
        addr = (struct mailimf_address *) clist_content(cur);
        switch (addr->ad_type) {
            case MAILIMF_ADDRESS_MAILBOX:
            {
                Address * address;
                
                address = Address::addressWithIMFMailbox(addr->ad_data.ad_mailbox);
                result->addObject(address);
                break;
            }
            
            case MAILIMF_ADDRESS_GROUP:
            {
                if (addr->ad_data.ad_group->grp_mb_list != NULL) {
                    Array * subArray;
                    
                    subArray = lep_address_list_from_lep_mailbox(addr->ad_data.ad_group->grp_mb_list);
                    result->addObjectsFromArray(subArray);
                }
                break;
            }
        }
    }
    
    return result;
}

static struct mailimf_mailbox_list * lep_mailbox_list_from_array(Array * addresses)
{
    struct mailimf_mailbox_list * mb_list;
    
    if (addresses == NULL)
        return NULL;
    
    if (addresses->count() == 0)
        return NULL;
    
    mb_list = mailimf_mailbox_list_new_empty();
    
    for(unsigned i = 0 ; i < addresses->count() ; i ++) {
        Address * address = (Address *) addresses->objectAtIndex(i);
        struct mailimf_mailbox * mailbox = address->createIMFMailbox();
        mailimf_mailbox_list_add(mb_list, mailbox);
    }
    
    return mb_list;
}

static struct mailimf_address_list * lep_address_list_from_array(Array * addresses)
{
    struct mailimf_address_list * addr_list;
    
    if (addresses == NULL)
        return NULL;
    
    if (addresses->count() == 0)
        return NULL;
    
    addr_list = mailimf_address_list_new_empty();

    for(unsigned i = 0 ; i < addresses->count() ; i ++) {
        Address * address = (Address *) addresses->objectAtIndex(i);
        struct mailimf_address * addr = address->createIMFAddress();
        mailimf_address_list_add(addr_list, addr);
    }
    
    return addr_list;
}

#pragma mark Message-ID conversion

static Array * msg_id_to_string_array(clist * msgids)
{
    clistiter * cur;
    Array * result;
    
    result = Array::array();
    
    for(cur = clist_begin(msgids) ; cur != NULL ; cur = clist_next(cur)) {
        char * msgid;
        String * str;
        
        msgid = (char *) clist_content(cur);
        str = String::stringWithUTF8Characters(msgid);
        result->addObject(str);
    }
    
    return result;
}

static clist * msg_id_from_string_array(Array * msgids)
{
    clist * result;
    
    if (msgids == NULL)
        return NULL;
    
    if (msgids->count() == 0)
        return NULL;
    
    result = clist_new();
    for(unsigned int i = 0 ; i < msgids->count() ; i ++) {
        String * msgid = (String *) msgids->objectAtIndex(i);
        clist_append(result, strdup(msgid->UTF8Characters()));
    }
    
    return result;
}

struct mailimf_fields * MessageHeader::createIMFFieldsAndFilterBcc(bool filterBcc)
{
    struct mailimf_date_time * imfDate;
    char * imfMsgid;
    char * imfSubject;
    struct mailimf_mailbox_list * imfFrom;
    struct mailimf_address_list * imfReplyTo;
    struct mailimf_address_list * imfTo;
    struct mailimf_address_list * imfCc;
    struct mailimf_address_list * imfBcc;
    clist * imfInReplyTo;
    clist * imfReferences;
    struct mailimf_fields * fields;
    
    imfDate = NULL;
    if (date() != (time_t) -1) {
        //MCLog("%lu", date());
        imfDate = dateFromTimestamp(date());
    }
    imfFrom = NULL;
    if (from() != NULL) {
        imfFrom = lep_mailbox_list_from_array(Array::arrayWithObject(from()));
    }
    imfReplyTo = lep_address_list_from_array(replyTo());
    imfTo = lep_address_list_from_array(to());
    imfCc = lep_address_list_from_array(cc());
    imfBcc = NULL;
    if (!filterBcc) {
        imfBcc = lep_address_list_from_array(bcc());
    }
    imfMsgid = NULL;
    if (messageID() != NULL) {
        imfMsgid = strdup(messageID()->UTF8Characters());
    }
    imfInReplyTo = msg_id_from_string_array(inReplyTo());
    imfReferences = msg_id_from_string_array(references());
    imfSubject = NULL;
    if ((subject() != NULL) && (subject()->length() > 0)) {
        Data * data;
        
        data = subject()->encodedMIMEHeaderValueForSubject();
        if (data->bytes() != NULL) {
            imfSubject = strdup(data->bytes());
        }
    }
    
    if ((imfTo == NULL) && (imfCc == NULL) && (imfBcc == NULL)) {
        imfTo = mailimf_address_list_new_empty();
        mailimf_address_list_add_parse(imfTo, (char *) "Undisclosed recipients:;");
    }
    
    fields = mailimf_fields_new_with_data_all(imfDate,
        imfFrom,
        NULL /* sender */,
        imfReplyTo,
        imfTo,
        imfCc,
        imfBcc,
        imfMsgid,
        imfInReplyTo,
        imfReferences,
        imfSubject);
    
    if (mExtraHeaders != NULL) {
        mc_foreachhashmapKeyAndValue(String, header, String, value, mExtraHeaders) {
            struct mailimf_field * field;
            
            field = mailimf_field_new_custom(strdup(header->UTF8Characters()), strdup(value->UTF8Characters()));
            mailimf_fields_add(fields, field);
        }
    }
    
    return fields;
}

extern "C" {
    extern int mailimap_hack_date_time_parse(char * str,
        struct mailimap_date_time ** result,
        size_t progr_rate,
        progress_function * progr_fun);
}

#pragma mark IMAP mailbox conversion

static Array * imap_mailbox_list_to_address_array(clist * imap_mailbox_list)
{
    clistiter * cur;
    Array * result;
    
    result = Array::array();
    
    for(cur = clist_begin(imap_mailbox_list) ; cur != NULL ;
        cur = clist_next(cur)) {
        struct mailimap_address * imap_addr;
        Address * address;
        
        imap_addr = (struct mailimap_address *) clist_content(cur);
        address = Address::addressWithIMAPAddress(imap_addr);
        result->addObject(address);
    }
    
    return result;
}

void MessageHeader::importIMAPEnvelope(struct mailimap_envelope * env)
{
    if (env->env_date != NULL) {
        size_t cur_token;
        struct mailimf_date_time * date_time;
        int r;

        cur_token = 0;
        r = mailimf_date_time_parse(env->env_date, strlen(env->env_date),
            &cur_token, &date_time);
        if (r == MAILIMF_NO_ERROR) {
            time_t timestamp;

            // date
            timestamp = timestampFromDate(date_time);
            setDate(timestamp);
            setReceivedDate(timestamp);
            mailimf_date_time_free(date_time);
        }
        else {
            struct mailimap_date_time * imap_date;

            r = mailimap_hack_date_time_parse(env->env_date, &imap_date, 0, NULL);
            if (r == MAILIMAP_NO_ERROR) {
                time_t timestamp;

                timestamp = timestampFromIMAPDate(imap_date);
                setDate(timestamp);
                setReceivedDate(timestamp);
                mailimap_date_time_free(imap_date);
            }
        }
    }

    if (env->env_subject != NULL) {
        char * subject;

        // subject
        subject = env->env_subject;
        setSubject(String::stringByDecodingMIMEHeaderValue(subject));
    }

    if (env->env_sender != NULL) {
        if (env->env_sender->snd_list != NULL) {
            Array * addresses;

            addresses = imap_mailbox_list_to_address_array(env->env_sender->snd_list);
            if (addresses->count() > 0) {
                setSender((Address *) addresses->objectAtIndex(0));
            }
        }
    }

    if (env->env_from != NULL) {
        if (env->env_from->frm_list != NULL) {
            Array * addresses;

            addresses = imap_mailbox_list_to_address_array(env->env_from->frm_list);
            if (addresses->count() > 0) {
                setFrom((Address *) addresses->objectAtIndex(0));
            }
        }
    }

        // skip Sender header

    if (env->env_reply_to != NULL) {
        if (env->env_reply_to->rt_list != NULL) {
            Array * addresses;

            addresses = imap_mailbox_list_to_address_array(env->env_reply_to->rt_list);
            setReplyTo(addresses);
        }
    }

    if (env->env_to != NULL) {
        if (env->env_to->to_list != NULL) {
            Array * addresses;

            addresses = imap_mailbox_list_to_address_array(env->env_to->to_list);
            setTo(addresses);
        }
    }

    if (env->env_cc != NULL) {
        if (env->env_cc->cc_list != NULL) {
            Array * addresses;

            addresses = imap_mailbox_list_to_address_array(env->env_cc->cc_list);
            setCc(addresses);
        }
    }

    if (env->env_bcc != NULL) {
        if (env->env_bcc->bcc_list != NULL) {
            Array * addresses;

            addresses = imap_mailbox_list_to_address_array(env->env_bcc->bcc_list);
            setBcc(addresses);
        }
    }

    if (env->env_in_reply_to != NULL) {
        size_t cur_token;
        clist * msg_id_list;
        int r;

        cur_token = 0;
        r = mailimf_msg_id_list_parse(env->env_in_reply_to,
            strlen(env->env_in_reply_to), &cur_token, &msg_id_list);
        if (r == MAILIMF_NO_ERROR) {
            Array * msgids;

            msgids = msg_id_to_string_array(msg_id_list);
            setInReplyTo(msgids);
            // in-reply-to
            clist_foreach(msg_id_list, (clist_func) mailimf_msg_id_free, NULL);
            clist_free(msg_id_list);
        }
    }

    if (env->env_message_id != NULL) {
        char * msgid;
        size_t cur_token;
        int r;

        cur_token = 0;
        r = mailimf_msg_id_parse(env->env_message_id, strlen(env->env_message_id),
            &cur_token, &msgid);
        if (r == MAILIMF_NO_ERROR) {
            // msg id
            String * str;

            str = String::stringWithUTF8Characters(msgid);
            setMessageID(str);
            mailimf_msg_id_free(msgid);
        }
    }
}

void MessageHeader::importIMAPReferences(Data * data)
{
    size_t cur_token;
    struct mailimf_fields * fields;
    int r;
    struct mailimf_single_fields single_fields;
    
    cur_token = 0;
    r = mailimf_fields_parse(data->bytes(), data->length(), &cur_token, &fields);
    if (r != MAILIMF_NO_ERROR) {
        return;
    }
    
    mailimf_single_fields_init(&single_fields, fields);
    if (single_fields.fld_references != NULL) {
        Array * msgids;
        
        msgids = msg_id_to_string_array(single_fields.fld_references->mid_list);
        setReferences(msgids);
    }
    if (single_fields.fld_subject != NULL) {
        if (single_fields.fld_subject->sbj_value != NULL) {
            bool broken;
            char * value;
            bool isASCII;
            
            broken = false;
            value = single_fields.fld_subject->sbj_value;
            
            isASCII = true;
            for(char * p = value ; * p != 0 ; p ++) {
                if ((unsigned char) * p >= 128) {
                    isASCII = false;
                }
            }
            if (strstr(value, "windows-1251") == NULL) {
                if (isASCII) {
                    broken = true;
                }
            }
            
            //MCLog("charset: %s %s", value, MCUTF8(charset));
            
            if (!broken) {
                setSubject(String::stringByDecodingMIMEHeaderValue(single_fields.fld_subject->sbj_value));
            }
        }
    }
    
    mailimf_fields_free(fields);
}

void MessageHeader::importIMAPInternalDate(struct mailimap_date_time * date)
{
    setReceivedDate(timestampFromIMAPDate(date));
}

Array * MessageHeader::recipientWithReplyAll(bool replyAll, bool includeTo, bool includeCc, Array * senderEmails)
{
    bool hasCc;
    bool hasTo;
    Set * addedAddresses;
    Array * toField;
    Array * ccField;
    bool containsSender;
    
    toField = NULL;
    ccField = NULL;
    
    hasTo = false;
    hasCc = false;
    addedAddresses = new Set();
    
    containsSender = false;
    if (senderEmails != NULL) {
      if (from() != NULL) {
        if (senderEmails->containsObject(from()->mailbox()->lowercaseString())) {
          containsSender = true;
        }
      }
      if (sender() != NULL) {
        if (senderEmails->containsObject(sender()->mailbox()->lowercaseString())) {
          containsSender = true;
        }
      }
    }
    
    if (containsSender) {
        Array * recipient;
        
        recipient = new Array();
        if (to() != NULL) {
            for(unsigned int i = 0 ; i < to()->count() ; i ++) {
                Address * address = (Address *) to()->objectAtIndex(i);
                if (addedAddresses->containsObject(address->mailbox()->lowercaseString())) {
                    continue;
                }
                if ((from() != NULL) && address->mailbox()->isEqualCaseInsensitive(from()->mailbox())) {
                    recipient->addObjectsFromArray(replyTo());
                    for(unsigned int j = 0 ; j < replyTo()->count() ; j ++) {
                        Address * rtAddress = (Address *) replyTo()->objectAtIndex(j);
                        if (addedAddresses->containsObject(rtAddress->mailbox()->lowercaseString())) {
                            continue;
                        }
                        addedAddresses->addObject(rtAddress->mailbox()->lowercaseString());
                    }
                }
                else {
                    if (address->mailbox() != NULL) {
                        recipient->addObject(address);
                        addedAddresses->addObject(address->mailbox()->lowercaseString());
                    }
                }
                hasTo = true;
            }
        }
        toField = recipient;
        toField->retain()->autorelease();
        recipient->release();
        
        if (replyAll) {
            recipient = new Array();
             if (cc() != NULL) {
               for(unsigned int i = 0 ; i < cc()->count() ; i ++) {
                    Address * address = (Address *) cc()->objectAtIndex(i);
                    if (addedAddresses->containsObject(address->mailbox()->lowercaseString())) {
                        continue;
                    }
                    if (address->mailbox() == NULL)
                        continue;
                    recipient->addObject(address);
                    addedAddresses->addObject(address->mailbox()->lowercaseString());
                    hasCc = true;
                }
            }
            ccField = recipient;
            ccField->retain()->autorelease();
            recipient->release();
        }
        
        if (!hasTo && !hasCc) {
            hasTo = true;
            toField = Array::arrayWithObject(from());
        }
    }
    else {
        addedAddresses->addObjectsFromArray(senderEmails);
        
        if (replyTo() != NULL && replyTo()->count() > 0) {
            hasTo = true;
            toField = replyTo();
            for(unsigned int i = 0 ; i < replyTo()->count() ; i ++) {
                Address * address = (Address *) replyTo()->objectAtIndex(i);
                if (address->mailbox() == NULL)
                    continue;
                addedAddresses->addObject(address->mailbox()->lowercaseString());
            }
        }
        else {
            if (from() != NULL && from()->mailbox() != NULL) {
                hasTo = true;
                toField = Array::arrayWithObject(from());
                addedAddresses->addObject(from()->mailbox()->lowercaseString());
            }
        }
        
        if (replyAll) {
            Array * recipient;
            
            recipient = new Array();
            if (to() != NULL) {
                for(unsigned int i = 0 ; i < to()->count() ; i ++) {
                    Address * address = (Address *) to()->objectAtIndex(i);
                    if (addedAddresses->containsObject(address->mailbox()->lowercaseString())) {
                        continue;
                    }
                    if (address->mailbox() == NULL)
                        continue;
                    recipient->addObject(address);
                    addedAddresses->addObject(address->mailbox()->lowercaseString());
                }
            }
            if (cc() != NULL) {
                for(unsigned int i = 0 ; i < cc()->count() ; i ++) {
                    Address * address = (Address *) cc()->objectAtIndex(i);
                    if (addedAddresses->containsObject(address->mailbox()->lowercaseString())) {
                        continue;
                    }
                    if (address->mailbox() == NULL)
                        continue;
                    recipient->addObject(address);
                    addedAddresses->addObject(address->mailbox()->lowercaseString());
                }
            }
            if (recipient->count() > 0) {
                hasCc = true;
            }
            ccField = recipient;
            ccField->retain()->autorelease();
            recipient->release();
        }
    }
    
    addedAddresses->release();
    
    Array * result;
    result = Array::array();
    if (hasTo && includeTo)
        result->addObjectsFromArray(toField);
    if (hasCc && includeCc)
        result->addObjectsFromArray(ccField);
    
    return result;
}

MessageHeader * MessageHeader::replyHeader(bool replyAll, Array * addressesExcludedFromRecipient)
{
    MessageHeader * result;
    String * subjectValue;
    Array * referencesValue;
    Array * inReplyTo;
    Array * toValue;
    Array * ccValue;
    
    referencesValue = NULL;
    inReplyTo = NULL;
    
    result = new MessageHeader();
    if (subject() == NULL) {
        subjectValue = MCSTR("Re: ");
    }
    else {
        subjectValue = MCSTR("Re: ")->stringByAppendingString(subject());
    }
    if (references() != NULL) {
        referencesValue = (Array *) (references()->copy());
        referencesValue->autorelease();
        if (messageID() != NULL ) {
            referencesValue->addObject(messageID());
        }
    }
    if (messageID()) {
        inReplyTo = Array::array();
        inReplyTo->addObject(messageID());
    }
    toValue = recipientWithReplyAll(replyAll, true, false, addressesExcludedFromRecipient);
    ccValue = recipientWithReplyAll(replyAll, false, true, addressesExcludedFromRecipient);;
    
    result->setSubject(subjectValue);
    result->setReferences(referencesValue);
    result->setInReplyTo(inReplyTo);
    result->setTo(toValue);
    result->setCc(ccValue);
    
    result->autorelease();
    return result;
}

MessageHeader * MessageHeader::forwardHeader()
{
    MessageHeader * result;
    String * subjectValue;
    Array * referencesValue;
    Array * inReplyTo;
    
    referencesValue = NULL;
    inReplyTo = NULL;

    result = new MessageHeader();
    if (subject() == NULL) {
        subjectValue = MCSTR("Fw: ");
    }
    else {
        subjectValue = MCSTR("Fw: ")->stringByAppendingString(subject());
    }
    if (references() != NULL) {
        referencesValue = (Array *) (references()->copy());
        referencesValue->autorelease();
        if (messageID() != NULL ) {
            referencesValue->addObject(messageID());
        }
    }
    if (messageID() != NULL) {
        inReplyTo = Array::array();
        inReplyTo->addObject(messageID());
    }
    result->setSubject(subjectValue);
    result->setReferences(referencesValue);
    result->setInReplyTo(inReplyTo);
    
    result->autorelease();
    return result;
}

HashMap * MessageHeader::serializable()
{
    HashMap * result = Object::serializable();
    
    if (messageID() != NULL) {
        result->setObjectForKey(MCSTR("messageID"), messageID());
    }
    if (references() != NULL) {
        result->setObjectForKey(MCSTR("references"), references());
    }
    if (inReplyTo() != NULL) {
        result->setObjectForKey(MCSTR("inReplyTo"), inReplyTo());
    }
    if (sender() != NULL) {
        result->setObjectForKey(MCSTR("sender"), sender()->serializable());
    }
    if (from() != NULL) {
        result->setObjectForKey(MCSTR("from"), from()->serializable());
    }
    if (to() != NULL) {
        result->setObjectForKey(MCSTR("to"), to()->serializable());
    }
    if (cc() != NULL) {
        result->setObjectForKey(MCSTR("cc"), cc()->serializable());
    }
    if (bcc() != NULL) {
        result->setObjectForKey(MCSTR("bcc"), bcc()->serializable());
    }
    if (replyTo() != NULL) {
        result->setObjectForKey(MCSTR("replyTo"), replyTo()->serializable());
    }
    if (subject() != NULL) {
        result->setObjectForKey(MCSTR("subject"), subject());
    }
    result->setObjectForKey(MCSTR("date"), String::stringWithUTF8Format("%lld", (unsigned long long) date()));
    result->setObjectForKey(MCSTR("receivedDate"), String::stringWithUTF8Format("%lld", (unsigned long long) receivedDate()));
    if (mExtraHeaders != NULL) {
        result->setObjectForKey(MCSTR("extraHeaders"), mExtraHeaders);
    }
    
    return result;
}

void MessageHeader::importSerializable(HashMap * hashmap)
{
    setMessageID((String *) hashmap->objectForKey(MCSTR("messageID")));
    setReferences((Array *) hashmap->objectForKey(MCSTR("references")));
    setInReplyTo((Array *) hashmap->objectForKey(MCSTR("inReplyTo")));
    setSender((Address *) Object::objectWithSerializable((HashMap *) hashmap->objectForKey(MCSTR("sender"))));
    setFrom((Address *) Object::objectWithSerializable((HashMap *) hashmap->objectForKey(MCSTR("from"))));
    setTo((Array *) Object::objectWithSerializable((HashMap *) hashmap->objectForKey(MCSTR("to"))));
    setCc((Array *)Object::objectWithSerializable((HashMap *) hashmap->objectForKey(MCSTR("cc"))));
    setBcc((Array *)Object::objectWithSerializable((HashMap *) hashmap->objectForKey(MCSTR("bcc"))));
    setReplyTo((Array *)Object::objectWithSerializable((HashMap *) hashmap->objectForKey(MCSTR("replyTo"))));
    setSubject((String *) hashmap->objectForKey(MCSTR("subject")));
    setDate((time_t) ((String *) hashmap->objectForKey(MCSTR("date")))->unsignedLongLongValue());
    setReceivedDate((time_t) ((String *) hashmap->objectForKey(MCSTR("receivedDate")))->unsignedLongLongValue());
    setExtraHeaders((HashMap *) hashmap->objectForKey(MCSTR("extraHeaders")));
}

static void * createObject()
{
    return new MessageHeader();
}

__attribute__((constructor))
static void initialize()
{
    Object::registerObjectConstructor("mailcore::MessageHeader", &createObject);
}
