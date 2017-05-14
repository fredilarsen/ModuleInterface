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


-- Dumping structure for table home_control.currentvalues
CREATE TABLE IF NOT EXISTS `currentvalues` (
  `id` varchar(10) NOT NULL,
  `value` varchar(12) DEFAULT NULL,
  `modified` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Data exporting was unselected.


-- Dumping structure for table home_control.settings
CREATE TABLE IF NOT EXISTS `settings` (
  `id` char(50) NOT NULL,
  `value` tinytext,
  `modified` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `description` tinytext,
  PRIMARY KEY (`id`),
  KEY `modified` (`modified`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ROW_FORMAT=COMPACT COMMENT='Contains the current settings, no history';

-- Data exporting was unselected.


-- Dumping structure for table home_control.timeseries
CREATE TABLE IF NOT EXISTS `timeseries` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `scan1m` tinyint(4) DEFAULT '0',
  `scan10m` tinyint(4) DEFAULT '0',
  `scan1h` tinyint(4) DEFAULT '0',
  `scan1d` tinyint(4) DEFAULT '0',
  `m1InactCnt` tinyint(4) unsigned DEFAULT NULL COMMENT 'Number of inactive or nonresponding modules',
  `m1FreeTot` smallint(5) unsigned DEFAULT NULL COMMENT 'Total free memory',
  `m1FreeMax` smallint(5) unsigned DEFAULT NULL COMMENT 'Size of largest free memory block',
  `m1FragCnt` tinyint(3) unsigned DEFAULT NULL COMMENT 'Number of memory fragments',
  `m1MemErr` tinyint(3) unsigned DEFAULT NULL COMMENT 'Out-of-memory status (0=OK, 1= bad)',
  `m1Uptime` int(10) unsigned DEFAULT NULL COMMENT 'Master uptime (s)',
  `m1UTC` int(10) unsigned DEFAULT NULL COMMENT 'Master system time (UTC in seconds)',
  `scUptime` int(10) unsigned DEFAULT NULL COMMENT 'Uptime (s) for Shunt Controller',
  `scTFlow` float DEFAULT NULL,
  `scTReturn` float DEFAULT NULL,
  `scTSupply` float DEFAULT NULL,
  `scTRoom` float DEFAULT NULL,
  `scTTarget` float DEFAULT NULL,
  `scTElec` float DEFAULT NULL,
  `scTServo` float DEFAULT NULL,
  `scTDrain` float DEFAULT NULL,
  `scPosShunt` float DEFAULT NULL,
  `scTOutLP` float DEFAULT NULL,
  `scPIDOut` float DEFAULT NULL,
  `scCurrMode` tinyint(4) DEFAULT NULL,
  `scLight` tinyint(3) unsigned DEFAULT NULL,
  `scStatBits` tinyint(3) unsigned DEFAULT NULL,
  `scLastLife` int(10) unsigned DEFAULT NULL,
  `scMemErr` tinyint(3) unsigned DEFAULT NULL,
  `e1Motion` tinyint(3) unsigned DEFAULT NULL,
  `e1OverHeat` tinyint(3) unsigned DEFAULT NULL,
  `e1UTC` int(10) unsigned DEFAULT NULL,
  `e1SunRise` smallint(5) unsigned DEFAULT NULL,
  `e1SunSet` smallint(5) unsigned DEFAULT NULL,
  `e1LigTarg` smallint(5) unsigned DEFAULT NULL,
  `e1LigCur` smallint(6) DEFAULT NULL,
  `e1LigCurLP` smallint(6) DEFAULT NULL,
  `e1State` tinyint(4) DEFAULT NULL,
  `e1Uptime` int(10) unsigned DEFAULT NULL,
  `e1LastLife` int(10) unsigned DEFAULT NULL,
  `e1MemErr` tinyint(3) unsigned DEFAULT NULL,
  `e1StatBits` tinyint(3) unsigned DEFAULT NULL COMMENT '1=Settings c mismatch, 2=Inputs c mismatch, 4=Missing settings, 8=Missing inputs, 16=Modified settings, 32=Missing time',
  `s1TElec` float DEFAULT NULL,
  `s1TTriac` float DEFAULT NULL,
  `s1TGarage` float DEFAULT NULL,
  `s1TOut` float DEFAULT NULL,
  `s1THumOut` float DEFAULT NULL,
  `s1TDew` float DEFAULT NULL,
  `s1HumOut` float DEFAULT NULL,
  `s1UTC` int(10) unsigned DEFAULT NULL,
  `s1PowTarg` tinyint(3) unsigned DEFAULT NULL,
  `s1PowCurr` tinyint(3) unsigned DEFAULT NULL,
  `s1PwmInt` tinyint(3) unsigned DEFAULT NULL,
  `s1TmDuty` tinyint(3) unsigned DEFAULT NULL,
  `s1Uptime` int(10) unsigned DEFAULT NULL,
  `s1LastLife` int(10) unsigned DEFAULT NULL,
  `s1MemErr` tinyint(3) unsigned DEFAULT NULL,
  `s1StatBits` tinyint(3) unsigned DEFAULT NULL,
  `ghUptime` int(10) unsigned DEFAULT NULL,
  `ghLastLife` int(10) unsigned DEFAULT NULL,
  `ghMemErr` tinyint(3) unsigned DEFAULT NULL,
  `ghTA` float DEFAULT NULL COMMENT 'Greenhouse Temperature Air',
  `ghTE` float DEFAULT NULL COMMENT 'Greenhouse Temperature Earth',
  `ghTElec` float DEFAULT NULL COMMENT 'Greenhouse Temperature Electronics',
  `ghTG` float DEFAULT NULL COMMENT 'Greenhouse Temperature Ground Level',
  `ghHA` float DEFAULT NULL COMMENT 'Greenhouse Humidity Air',
  `ghHE` float DEFAULT NULL COMMENT 'Greenhouse Humidity Earth',
  `ghRain` float DEFAULT NULL COMMENT 'Greenhouse Rain Indoor (bool)',
  `ghLight` float DEFAULT NULL COMMENT 'Greenhouse Light Sensor',
  `ghDewPoint` float DEFAULT NULL COMMENT 'Greenhouse Dewpoint Calculated',
  `ghMotion` tinyint(4) DEFAULT NULL COMMENT 'Greenhoouse Motion Detected (bool)',
  `ghStatBits` tinyint(4) unsigned DEFAULT NULL,
  `c1UTC` int(10) unsigned DEFAULT NULL COMMENT 'Car heater 1 system time (UTC)',
  `c1LastLife` int(10) unsigned DEFAULT NULL COMMENT 'Car heater 1 time since last life sign (s)',
  `c1MemErr` tinyint(3) unsigned DEFAULT NULL COMMENT 'Car heater 1 out of memory (0/1)',
  `c1Uptime` int(10) unsigned DEFAULT NULL COMMENT 'Car heater 1 uptime (s)',
  `c1PowCurr` tinyint(3) unsigned DEFAULT NULL COMMENT 'Car heater 1 outlet power (0% or 100%)',
  `c1CommQual` tinyint(3) unsigned DEFAULT NULL COMMENT 'Car heater 1 communication quality (%)',
  `c1Reason` tinyint(3) unsigned DEFAULT NULL COMMENT 'Car heater 1 reason for change (1=button, 2=web, 3=calendar)',
  `c1TmUntil` int(10) unsigned DEFAULT NULL COMMENT 'Car heater 1 current end time (UTC)',
  `c1StatBits` tinyint(3) unsigned DEFAULT NULL,
  `c2UTC` int(10) unsigned DEFAULT NULL COMMENT 'Car heater 2 system time (UTC)',
  `c2LastLife` int(10) unsigned DEFAULT NULL COMMENT 'Car heater 2 time since last life sign (s)',
  `c2MemErr` tinyint(3) unsigned DEFAULT NULL COMMENT 'Car heater 2 out of memory (0/1)',
  `c2Uptime` int(10) unsigned DEFAULT NULL COMMENT 'Car heater 2 uptime (s)',
  `c2PowCurr` tinyint(3) unsigned DEFAULT NULL COMMENT 'Car heater 2 outlet power (0% or 100%)',
  `c2CommQual` tinyint(3) unsigned DEFAULT NULL COMMENT 'Car heater 2 communication quality (%)',
  `c2TmUntil` int(10) unsigned DEFAULT NULL COMMENT 'Car heater 2 current end time (UTC)',
  `c2Reason` tinyint(3) unsigned DEFAULT NULL COMMENT 'Car heater 2 reason for change (1=button, 2=web, 3=calendar)',
  `c2StatBits` tinyint(3) unsigned DEFAULT NULL,
  `c3PowCurr` tinyint(3) unsigned DEFAULT NULL,
  `c3CommQual` tinyint(3) unsigned DEFAULT NULL,
  `c3MemErr` tinyint(3) unsigned DEFAULT NULL,
  `c3TmUntil` int(10) unsigned DEFAULT NULL,
  `c3UTC` int(10) unsigned DEFAULT NULL,
  `c3LastLife` int(10) unsigned DEFAULT NULL,
  `c3Reason` tinyint(3) unsigned DEFAULT NULL,
  `c3Uptime` int(10) unsigned DEFAULT NULL,
  `c3StatBits` tinyint(3) unsigned DEFAULT NULL,
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
