﻿using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

namespace ApiServer.Model.Entity
{
    [Table("user")]
    public class UserEntity
    {
        [Key]
        [Column("id")]
        public long Id { get; set; }
        [Column("user_id")]
        public string UserId { get; set; }
        [Column("password")]
        public string Password { get; set; }
        [Column("username")]
        public string Username { get; set; }
        [Column("email")]
        public string Email { get; set; }
        [Column("is_alive")]
        public string IsAlive { get; set; }

    }
}
