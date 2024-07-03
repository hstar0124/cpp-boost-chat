#include "MySQLManager.h"

MySQLManager::MySQLManager(const std::string& host, const std::string& user, const std::string& password, const std::string& db, unsigned int port)
    : m_Conn(mysql_init(nullptr), mysql_close)
{

    if (!m_Conn) 
    {
        throw std::runtime_error("MySQL initialization failed");
    }

    if (!mysql_real_connect(m_Conn.get(), host.c_str(), user.c_str(), password.c_str(), db.c_str(), port, nullptr, 0)) 
    {
        throw std::runtime_error(mysql_error(m_Conn.get()));
    }
}

bool MySQLManager::AddFriendRequest(const std::string& sender_id, const std::string& receiver_id)
{
    std::string query = "INSERT INTO friend_requests (sender_id, receiver_id, status) VALUES (?, ?, 'P')";
    std::vector<std::string> params = { sender_id, receiver_id };

    try 
    {
        executePreparedStatement(query, params);
    }
    catch (const std::exception& e) 
    {
        throw std::runtime_error("Failed to add user: " + std::string(e.what()));
    }
}

void MySQLManager::executePreparedStatement(const std::string& query, std::vector<std::string>& params)
{
    std::unique_ptr<MYSQL_STMT, decltype(&mysql_stmt_close)> stmt(mysql_stmt_init(m_Conn.get()), mysql_stmt_close);

    if (!stmt)
    {
        throw std::runtime_error("Failed to initialize statement");
    }

    if (mysql_stmt_prepare(stmt.get(), query.c_str(), query.size()) != 0)
    {
        throw std::runtime_error(mysql_stmt_error(stmt.get()));
    }

    std::vector<MYSQL_BIND> bindParams(params.size());
    std::vector<std::string> paramValues(params.size());
    for (size_t i = 0; i < params.size(); ++i)
    {
        paramValues[i] = params[i];
        bindParams[i].buffer_type = MYSQL_TYPE_STRING;
        bindParams[i].buffer = (char*)paramValues[i].c_str();
        bindParams[i].buffer_length = paramValues[i].size();
        bindParams[i].is_null = 0;
    }

    if (mysql_stmt_bind_param(stmt.get(), bindParams.data()) != 0)
    {
        throw std::runtime_error(mysql_stmt_error(stmt.get()));
    }

    if (mysql_stmt_execute(stmt.get()) != 0)
    {
        throw std::runtime_error(mysql_stmt_error(stmt.get()));
    }

    if (mysql_stmt_store_result(stmt.get()) != 0)
    {
        throw std::runtime_error(mysql_stmt_error(stmt.get()));
    }

    int numRowsAffected = mysql_stmt_affected_rows(stmt.get());

    if (numRowsAffected < 1)
    {
        throw std::runtime_error("Failed to get number of rows affected");
    }

    std::cout << "Number of rows affected: " << numRowsAffected << std::endl;
}

void MySQLManager::UpdateFriend(const std::string& senderId, const std::string& receiverId, const std::string& status)
{
    std::string query = "UPDATE friend_requests SET status = ? WHERE sender_id = ? AND receiver_id = ?";
    std::vector<std::string> params = { status, senderId, receiverId };

    try 
    {
        executePreparedStatement(query, params);
    }
    catch (const std::exception& e) 
    {
        throw std::runtime_error("Failed to update user: " + std::string(e.what()));
    }
}

void MySQLManager::DeleteFriendRequest(const std::string& sender_id, const std::string& receiver_id)
{
    std::string query = "DELETE FROM friend_requests WHERE sender_id = ? AND receiver_id = ?";
    std::vector<std::string> params = { sender_id, receiver_id };

    try
    {
        executePreparedStatement(query, params);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to update user: " + std::string(e.what()));
    }
}


bool MySQLManager::AddFriendship(const std::string& user_id1, const std::string& user_id2)
{
    std::string query = "INSERT INTO friendships (user_id1, user_id2) VALUES (?, ?)";
    std::vector<std::string> params = { user_id1, user_id2 };

    try
    {
        executePreparedStatement(query, params);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error("Failed to add user: " + std::string(e.what()));
    }
}

bool MySQLManager::HasFriendRequest(const std::string& sender_id, const std::string& receiver_id)
{
    try
    {
        std::string query = "SELECT COUNT(*) AS count FROM friend_requests WHERE sender_id = ? AND receiver_id = ? AND (status = 'P' OR status = 'A')";
        std::vector<std::string> params = { sender_id, receiver_id };
        int count = 0;

        auto stmt = mysql_stmt_init(m_Conn.get());
        if (!stmt)
        {
            throw std::runtime_error("Failed to initialize statement");
        }

        if (mysql_stmt_prepare(stmt, query.c_str(), query.size()) != 0)
        {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }

        std::vector<MYSQL_BIND> bindParams(params.size());
        std::vector<std::string> paramValues(params.size());
        for (size_t i = 0; i < params.size(); ++i)
        {
            paramValues[i] = params[i];
            bindParams[i].buffer_type = MYSQL_TYPE_STRING;
            bindParams[i].buffer = (char*)paramValues[i].c_str();
            bindParams[i].buffer_length = paramValues[i].size();
            bindParams[i].is_null = 0;
        }

        if (mysql_stmt_bind_param(stmt, bindParams.data()) != 0)
        {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }

        if (mysql_stmt_execute(stmt) != 0)
        {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }

        MYSQL_BIND result;
        memset(&result, 0, sizeof(result));
        result.buffer_type = MYSQL_TYPE_LONG;
        result.buffer = &count;

        if (mysql_stmt_bind_result(stmt, &result) != 0)
        {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }

        if (mysql_stmt_fetch(stmt) != 0)
        {
            throw std::runtime_error(mysql_stmt_error(stmt));
        }

        mysql_stmt_free_result(stmt);
        mysql_stmt_close(stmt);

        return count > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error in HasPendingFriendRequest: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<UserEntity> MySQLManager::GetUserById(const std::string& requestId) 
{
    std::string query = "SELECT id, user_id, password, username, email, is_alive FROM user WHERE id = ?";
    std::vector<std::string> params = { requestId };

    MYSQL_STMT* stmt = mysql_stmt_init(m_Conn.get());
    if (!stmt) 
    {
        throw std::runtime_error("Failed to initialize statement");
    }

    if (mysql_stmt_prepare(stmt, query.c_str(), query.size()) != 0) 
    {
        throw std::runtime_error(mysql_stmt_error(stmt));
    }

    std::vector<MYSQL_BIND> bindParams(params.size());
    std::vector<std::string> paramValues(params.size());
    for (size_t i = 0; i < params.size(); ++i) 
    {
        paramValues[i] = params[i];
        bindParams[i].buffer_type = MYSQL_TYPE_STRING;
        bindParams[i].buffer = (char*)paramValues[i].c_str();
        bindParams[i].buffer_length = paramValues[i].size();
        bindParams[i].is_null = 0;
    }

    if (mysql_stmt_bind_param(stmt, bindParams.data()) != 0) 
    {
        throw std::runtime_error(mysql_stmt_error(stmt));
    }

    if (mysql_stmt_execute(stmt) != 0) 
    {
        throw std::runtime_error(mysql_stmt_error(stmt));
    }

    std::shared_ptr<UserEntity> user = std::make_shared<UserEntity>();

    MYSQL_BIND result[6];
    memset(result, 0, sizeof(result));

    uint64_t id;
    char userId[100];
    char password[255];
    char username[100];
    char email[255];
    char isAlive;

    result[0].buffer_type = MYSQL_TYPE_LONGLONG;
    result[0].buffer = &id;

    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = userId;
    result[1].buffer_length = sizeof(userId);

    result[2].buffer_type = MYSQL_TYPE_STRING;
    result[2].buffer = password;
    result[2].buffer_length = sizeof(password);

    result[3].buffer_type = MYSQL_TYPE_STRING;
    result[3].buffer = username;
    result[3].buffer_length = sizeof(username);

    result[4].buffer_type = MYSQL_TYPE_STRING;
    result[4].buffer = email;
    result[4].buffer_length = sizeof(email);

    result[5].buffer_type = MYSQL_TYPE_STRING;
    result[5].buffer = &isAlive;
    result[5].buffer_length = sizeof(isAlive);

    if (mysql_stmt_bind_result(stmt, result) != 0) 
    {
        throw std::runtime_error(mysql_stmt_error(stmt));
    }

    if (mysql_stmt_fetch(stmt) == 0) 
    {
        user->SetId(id);
        user->SetUserId(userId);
        //user->SetPassword(password);
        user->SetUsername(username);
        user->SetEmail(email);
        user->SetIsAlive(isAlive);
    }
    else 
    {
        throw std::runtime_error("User with id " + requestId + " not found");
    }

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);

    return user;
}

std::shared_ptr<UserEntity> MySQLManager::GetUserByConditions(const std::vector<Condition>& conditions)
{
    std::string query = "SELECT id, user_id, password, username, email, is_alive FROM user WHERE ";
    std::vector<std::string> params;

    for (size_t i = 0; i < conditions.size(); ++i)
    {
        if (i > 0) query += " AND ";
        query += conditions[i].column + " = ?";
        params.push_back(conditions[i].value);
    }

    MYSQL_STMT* stmt = mysql_stmt_init(m_Conn.get());
    if (!stmt)
    {
        throw std::runtime_error("Failed to initialize statement");
    }

    if (mysql_stmt_prepare(stmt, query.c_str(), query.size()) != 0)
    {
        throw std::runtime_error(mysql_stmt_error(stmt));
    }

    std::vector<MYSQL_BIND> bindParams(params.size());
    std::vector<std::string> paramValues(params.size());
    for (size_t i = 0; i < params.size(); ++i)
    {
        paramValues[i] = params[i];
        bindParams[i].buffer_type = MYSQL_TYPE_STRING;
        bindParams[i].buffer = (char*)paramValues[i].c_str();
        bindParams[i].buffer_length = paramValues[i].size();
        bindParams[i].is_null = 0;
    }

    if (mysql_stmt_bind_param(stmt, bindParams.data()) != 0)
    {
        throw std::runtime_error(mysql_stmt_error(stmt));
    }

    if (mysql_stmt_execute(stmt) != 0)
    {
        throw std::runtime_error(mysql_stmt_error(stmt));
    }

    std::shared_ptr<UserEntity> user = std::make_shared<UserEntity>();

    MYSQL_BIND result[6];
    memset(result, 0, sizeof(result));

    uint64_t id;
    char userId[100];
    char password[255];
    char username[100];
    char email[255];
    char isAlive;

    result[0].buffer_type = MYSQL_TYPE_LONGLONG;
    result[0].buffer = &id;

    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = userId;
    result[1].buffer_length = sizeof(userId);

    result[2].buffer_type = MYSQL_TYPE_STRING;
    result[2].buffer = password;
    result[2].buffer_length = sizeof(password);

    result[3].buffer_type = MYSQL_TYPE_STRING;
    result[3].buffer = username;
    result[3].buffer_length = sizeof(username);

    result[4].buffer_type = MYSQL_TYPE_STRING;
    result[4].buffer = email;
    result[4].buffer_length = sizeof(email);

    result[5].buffer_type = MYSQL_TYPE_STRING;
    result[5].buffer = &isAlive;
    result[5].buffer_length = sizeof(isAlive);

    if (mysql_stmt_bind_result(stmt, result) != 0)
    {
        throw std::runtime_error(mysql_stmt_error(stmt));
    }

    if (mysql_stmt_fetch(stmt) == 0)
    {
        user->SetId(id);
        user->SetUserId(userId);
        //user->SetPassword(password);
        user->SetUsername(username);
        user->SetEmail(email);
        user->SetIsAlive(isAlive);
    }
    else
    {
        throw std::runtime_error("User not found with given conditions");
    }

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);

    return user;
}


void MySQLManager::BeginTransaction()
{
    if (mysql_autocommit(m_Conn.get(), false) != 0)
    {
        throw std::runtime_error("Failed to start transaction: " + std::string(mysql_error(m_Conn.get())));
    }
}

void MySQLManager::CommitTransaction()
{
    if (mysql_commit(m_Conn.get()) != 0)
    {
        throw std::runtime_error("Failed to commit transaction: " + std::string(mysql_error(m_Conn.get())));
    }
    mysql_autocommit(m_Conn.get(), true); // Restore autocommit mode
}

void MySQLManager::RollbackTransaction()
{
    if (mysql_rollback(m_Conn.get()) != 0)
    {
        throw std::runtime_error("Failed to rollback transaction: " + std::string(mysql_error(m_Conn.get())));
    }
    mysql_autocommit(m_Conn.get(), true); // Restore autocommit mode
}