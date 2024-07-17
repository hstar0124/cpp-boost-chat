using ApiServer.CustomException;
using LoginApiServer.Repository.Interface;
using StackExchange.Redis;

namespace LoginApiServer.Repository
{
    public class CacheRepositoryFromRedis : ICacheRepository
    {
        private readonly ILogger<CacheRepositoryFromRedis> _logger;
        private readonly ConnectionMultiplexer _redis;
        private readonly int _sessionDbIndex = 0;
        private readonly IDatabase _sessionDb;

        public CacheRepositoryFromRedis(ILogger<CacheRepositoryFromRedis> logger)
        {
            _logger = logger;

            try
            {
                _redis = ConnectionMultiplexer.Connect("127.0.0.1");
                _sessionDb = _redis.GetDatabase(_sessionDbIndex);
                _logger.LogInformation("Connected to Redis successfully");
            }
            catch (Exception ex)
            {
                _logger.LogError($"Error connecting to Redis: {ex.Message}");
                throw new ServerException("Unexpected server error occurred");
            }
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

                    // 트랜잭션 내 비동기 작업을 동기식으로 수행
                    var deleteOldSession = tran.KeyDeleteAsync(existingSessionKey);
                    var setNewSession = tran.StringSetAsync(sessionKey, accountId, new TimeSpan(0, 3, 0));
                    var setNewAccountKey = tran.StringSetAsync(accountKey, sessionId, new TimeSpan(0, 3, 0));

                    // 트랜잭션 실행
                    bool committed = await tran.ExecuteAsync();
                    await Task.WhenAll(deleteOldSession, setNewSession, setNewAccountKey);

                    return committed ? UserStatusCode.Success : UserStatusCode.Failure;
                }
                else
                {
                    // 새로운 세션을 생성
                    var tran = _sessionDb.CreateTransaction();

                    tran.AddCondition(Condition.KeyNotExists(sessionKey));
                    tran.AddCondition(Condition.KeyNotExists(accountKey));

                    // 트랜잭션 내 비동기 작업을 동기식으로 수행
                    var setNewSession = tran.StringSetAsync(sessionKey, accountId, new TimeSpan(0, 3, 0));
                    var setNewAccountKey = tran.StringSetAsync(accountKey, sessionId, new TimeSpan(0, 3, 0));

                    // 트랜잭션 실행
                    bool committed = await tran.ExecuteAsync();
                    await Task.WhenAll(setNewSession, setNewAccountKey);

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
