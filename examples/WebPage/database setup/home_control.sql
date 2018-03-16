-- phpMyAdmin SQL Dump
-- version 4.6.6deb5
-- https://www.phpmyadmin.net/
--
-- Host: localhost:3306
-- Generation Time: 16. Mar, 2018 23:53 PM
-- Server-versjon: 5.7.21-0ubuntu0.17.10.1
-- PHP Version: 7.1.11-0ubuntu0.17.10.1

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `home_control`
--
CREATE DATABASE IF NOT EXISTS `home_control` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;
USE `home_control`;

-- --------------------------------------------------------

--
-- Tabellstruktur for tabell `currentvalues`
--

CREATE TABLE `currentvalues` (
  `id` varchar(10) NOT NULL,
  `value` varchar(12) DEFAULT NULL,
  `modified` int(11) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Contains the latest outputs, no history';

--
-- Dataark for tabell `currentvalues`
--

INSERT INTO `currentvalues` (`id`, `value`, `modified`) VALUES
('lcLastLife', '0', 1521240833),
('lcLightOn', '0', 1521240833),
('lcMemErr', '0', 1521240833),
('lcStatBits', '0', 1521240833),
('lcUptime', '136059', 1521240833),
('lcUTC', '1521240832', 1521240833),
('m1FragCnt', '0', 1521240833),
('m1FreeMax', '0', 1521240833),
('m1FreeTot', '0', 1521240833),
('m1InactCnt', '0', 1521240833),
('m1MemErr', '0', 1521240833),
('m1Uptime', '1000', 1521240833),
('m1UTC', '1521240832', 1521240833),
('scan10m', '0', 1521240833),
('scan1d', '0', 1521240833),
('scan1h', '0', 1521240833),
('scan1m', '0', 1521240833),
('smLastLife', '0', 1521240833),
('smLight', '176', 1521240833),
('smLightLP', '202.1837921', 1521240833),
('smMemErr', '0', 1521240833),
('smMotion', '0', 1521240833),
('smStatBits', '0', 1521240833),
('smUptime', '132853', 1521240833);

-- --------------------------------------------------------

--
-- Tabellstruktur for tabell `settings`
--

CREATE TABLE `settings` (
  `id` char(50) NOT NULL,
  `value` text,
  `modified` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `description` tinytext
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Contains the current settings, no history' ROW_FORMAT=COMPACT;

--
-- Dataark for tabell `settings`
--

INSERT INTO `settings` (`id`, `value`, `modified`, `description`) VALUES
('lcLimit', '150', '2017-12-21 17:23:16', 'Ambient light threshold before activating LED'),
('lcMode', '2', '2018-03-11 22:11:56', 'Controller mode (Off=0/On=1/Auto=2)'),
('lcTEndM', '0', '2017-12-04 19:18:28', 'Minute of day, end of interval to keep light on'),
('lcTStartM', '0', '2017-12-04 19:18:26', 'Minutes of day, start of interval to keep light on'),
('m1DevID', '1', '2018-03-15 20:42:16', 'PJON device id for the master'),
('m1IntOutputs', '1000', '2018-03-15 20:36:28', 'Transfer interval in ms for outputs'),
('m1IntSettings', '1000', '2018-03-15 20:36:28', 'Transfer interval in ms for settings'),
('m1Modules', 'SensMon:sm:10 LightCon:lc:20', '2018-03-15 20:40:59', 'The list of modules to synchronize, in Name:pf:id format'),
('smMode', '0', '2018-03-08 19:05:17', 'Controller mode (Off=0/On=1/Auto=2)'),
('wpixResolution', '-', '2018-03-16 22:50:52', 'Last selected web page plot resolution');

-- --------------------------------------------------------

--
-- Tabellstruktur for tabell `timeseries`
--

CREATE TABLE `timeseries` (
  `id` bigint(20) NOT NULL,
  `time` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `scan1m` tinyint(4) DEFAULT '0',
  `scan10m` tinyint(4) DEFAULT '0',
  `scan1h` tinyint(4) DEFAULT '0',
  `scan1d` tinyint(4) DEFAULT '0',
  `m1UTC` int(11) NOT NULL COMMENT 'Master system time ( Coordinated Universal Time)',
  `m1Uptime` int(10) UNSIGNED DEFAULT NULL COMMENT 'Master uptime (s)',
  `m1InactCnt` tinyint(4) UNSIGNED DEFAULT NULL COMMENT 'Number of inactive or nonresponding modules',
  `m1FreeTot` smallint(5) UNSIGNED DEFAULT NULL COMMENT 'Total free memory',
  `m1FreeMax` smallint(5) UNSIGNED DEFAULT NULL COMMENT 'Size of largest free memory block',
  `m1FragCnt` tinyint(3) UNSIGNED DEFAULT NULL COMMENT 'Number of memory fragments',
  `m1MemErr` tinyint(3) UNSIGNED DEFAULT NULL COMMENT 'Out-of-memory status (0=OK, 1= bad)',
  `lcUTC` int(10) UNSIGNED DEFAULT NULL,
  `lcUptime` int(10) UNSIGNED DEFAULT NULL,
  `lcLastLife` int(10) UNSIGNED DEFAULT NULL,
  `lcLightOn` tinyint(4) NOT NULL,
  `smUptime` int(10) UNSIGNED DEFAULT NULL,
  `smLastLife` int(11) UNSIGNED DEFAULT NULL,
  `smMotion` tinyint(3) UNSIGNED DEFAULT NULL,
  `smLight` smallint(6) NOT NULL COMMENT 'Measured ambient light',
  `smLightLP` float NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='All outputs logged versus time';

--
-- Indexes for dumped tables
--

--
-- Indexes for table `currentvalues`
--
ALTER TABLE `currentvalues`
  ADD PRIMARY KEY (`id`);

--
-- Indexes for table `settings`
--
ALTER TABLE `settings`
  ADD PRIMARY KEY (`id`),
  ADD KEY `modified` (`modified`);

--
-- Indexes for table `timeseries`
--
ALTER TABLE `timeseries`
  ADD PRIMARY KEY (`time`),
  ADD UNIQUE KEY `id` (`id`),
  ADD KEY `scan1m` (`scan1m`,`time`),
  ADD KEY `scan10m` (`scan10m`,`time`),
  ADD KEY `scan1h` (`scan1h`,`time`),
  ADD KEY `scan1d` (`scan1d`,`time`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `timeseries`
--
ALTER TABLE `timeseries`
  MODIFY `id` bigint(20) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=760619;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
