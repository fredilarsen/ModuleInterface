-- --------------------------------------------------------
-- Host:                         127.0.0.1
-- Server version:               10.1.18-MariaDB - mariadb.org binary distribution
-- Server OS:                    Win32
-- HeidiSQL Version:             9.3.0.4984
-- --------------------------------------------------------

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8mb4 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;

-- Dumping database structure for home_control
CREATE DATABASE IF NOT EXISTS `home_control` /*!40100 DEFAULT CHARACTER SET utf8 */;
USE `home_control`;


-- Dumping structure for table home_control.settings
CREATE TABLE IF NOT EXISTS `settings` (
  `id` char(50) NOT NULL,
  `value` tinytext,
  `description` tinytext,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Contains the current settings, no history';

-- Data exporting was unselected.


-- Dumping structure for table home_control.timeseries
CREATE TABLE IF NOT EXISTS `timeseries` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `scan1m` tinyint(4) DEFAULT '0',
  `scan10m` tinyint(4) DEFAULT '0',
  `scan1h` tinyint(4) DEFAULT '0',
  `scan1d` tinyint(4) DEFAULT '0',
  `scTime` int(11) DEFAULT NULL,
  `scUptime` float DEFAULT NULL COMMENT 'Uptime (hours) for Shunt Controller',
  `scTempFlow` float DEFAULT NULL,
  `scTempReturn` float DEFAULT NULL,
  `scTempSupply` float DEFAULT NULL,
  `scKp` float DEFAULT NULL,
  `scKi` float DEFAULT NULL,
  `scKd` float DEFAULT NULL,
  `ghTA` float DEFAULT NULL COMMENT 'Greenhouse Temperature Air',
  `ghTE` float DEFAULT NULL COMMENT 'Greenhouse Temperature Earth',
  `ghTElec` float DEFAULT NULL COMMENT 'Greenhouse Temperature Electronics',
  `ghTG` float DEFAULT NULL COMMENT 'Greenhouse Temperature Ground Level',
  `ghHA` float DEFAULT NULL COMMENT 'Greenhouse Humidity Air',
  `ghHE` float DEFAULT NULL COMMENT 'Greenhouse Humidity Earth',
  `ghRain` float DEFAULT NULL COMMENT 'Greenhouse Rain Indoor (bool)',
  `ghLigt` float DEFAULT NULL COMMENT 'Greenhouse Light Sensor',
  `ghDP` float DEFAULT NULL COMMENT 'Greenhouse Dewpoint Calculated',
  `ghMotion` tinyint(4) DEFAULT NULL COMMENT 'Greenhoouse Motion Detected (bool)',
  PRIMARY KEY (`time`),
  UNIQUE KEY `id` (`id`),
  KEY `scan1m` (`scan1m`,`time`),
  KEY `scan10m` (`scan10m`,`time`),
  KEY `scan1h` (`scan1h`,`time`),
  KEY `scan1d` (`scan1d`,`time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='All sensor measurements logged versus time';

-- Data exporting was unselected.
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IF(@OLD_FOREIGN_KEY_CHECKS IS NULL, 1, @OLD_FOREIGN_KEY_CHECKS) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
