using LoginApiServer.Model;
using LoginApiServer.Repository.Interface;
using StackExchange.Redis;

namespace LoginApiServer.Repository
{
    public class CacheRepositoryFromRedis : ICacheRepository
    {
        private readonly ConnectionMultiplexer _redis;
        private readonly int _sessionDbIndex = 0;
        private readonly IDatabase _sessionDb;

        public CacheRepositoryFromRedis()
        {
            _redis = ConnectionMultiplexer.Connect("127.0.0.1");
            _sessionDb = _redis.GetDatabase(_sessionDbIndex);
        }

        public async Task<UserStatusCode> CreateSession(string sessionId, long accountId)
        {
            try
            {
                var accountKey = $"Account:{accountId}";
                var sessionKey = $"Session:{sessionId}";

                // 먼저 AccountID로 조회
                RedisValue existingSessionId = await _sessionDb.StringGetAsync(accountKey);
                if (existingSessionId.HasValue)
                {
                    var existingSessionKey = $"Session:{existingSessionId}";

                    var tran = _sessionDb.CreateTransaction();

                    // 기존 세션 삭제 및 새로운 세션 설정
                    await tran.KeyDeleteAsync(existingSessionKey);
                    await tran.StringSetAsync(sessionKey, accountId, new TimeSpan(0, 3, 0));
                    await tran.StringSetAsync(accountKey, sessionId, new TimeSpan(0, 3, 0));

                    bool committed = await tran.ExecuteAsync();
                    return committed ? UserStatusCode.Success : UserStatusCode.Failure;
                }
                else
                {
                    // 새로운 세션을 생성
                    var tran = _sessionDb.CreateTransaction();

                    tran.AddCondition(Condition.KeyNotExists(sessionKey));
                    tran.AddCondition(Condition.KeyNotExists(accountKey));

                    await tran.StringSetAsync(sessionKey, accountId, new TimeSpan(0, 3, 0));
                    await tran.StringSetAsync(accountKey, sessionId, new TimeSpan(0, 3, 0));

                    bool committed = await tran.ExecuteAsync();
                    return committed ? UserStatusCode.Success : UserStatusCode.Failure;
                }
            }
            catch (Exception)
            {
                return UserStatusCode.ServerError;
            }
        }

        public async Task<UserStatusCode> KeepAliveSessionFromAccountId(long accountId)
        {
            try
            {
                string accountKey = $"Account:{accountId}";
                RedisValue redisValue = await _sessionDb.StringGetAsync(accountKey);
                if (!redisValue.HasValue)
                {
                    return UserStatusCode.Failure;
                }

                var sessionId = redisValue.ToString();
                var sessionKey = $"Session:{sessionId}";

                var tran = _sessionDb.CreateTransaction();

                await tran.KeyExpireAsync(accountKey, new TimeSpan(0, 3, 0));
                await tran.KeyExpireAsync(sessionKey, new TimeSpan(0, 3, 0));

                bool committed = await tran.ExecuteAsync();
                return committed ? UserStatusCode.Success : UserStatusCode.Failure;
            }
            catch (Exception)
            {
                return UserStatusCode.ServerError;
            }
        }

        public async Task<UserStatusCode> DeleteSessionFromAccountId(long accountId)
        {
            try
            {
                string accountKey = $"Account:{accountId}";
                RedisValue redisValue = await _sessionDb.StringGetAsync(accountKey);
                if (!redisValue.HasValue)
                {
                    return UserStatusCode.Failure;
                }

                string sessionId = redisValue.ToString();
                string sessionKey = $"Session:{sessionId}";

                var tran = _sessionDb.CreateTransaction();

                await tran.KeyDeleteAsync(sessionKey);
                await tran.KeyDeleteAsync(accountKey);

                bool committed = await tran.ExecuteAsync();
                return committed ? UserStatusCode.Success : UserStatusCode.Failure;
            }
            catch (Exception)
            {
                return UserStatusCode.ServerError;
            }
        }

    }
}
