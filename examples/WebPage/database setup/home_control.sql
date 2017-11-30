-- phpMyAdmin SQL Dump
-- version 4.7.4
-- https://www.phpmyadmin.net/
--
-- Host: 127.0.0.1
-- Generation Time: 30. Nov, 2017 14:48 PM
-- Server-versjon: 10.1.28-MariaDB
-- PHP Version: 7.1.11

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET AUTOCOMMIT = 0;
START TRANSACTION;
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
-- Tabellstruktur for tabell `contracts`
--

CREATE TABLE `contracts` (
  `moduleprefix` varchar(2) DEFAULT NULL,
  `modulename` varchar(10) DEFAULT NULL,
  `contract` varchar(250) DEFAULT NULL,
  `updated` timestamp NULL DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
('lcLastLife', '0', 1512049438),
('lcLightOn', '0', 1512049438),
('lcMemErr', '0', 1512049438),
('lcStatBits', '0', 1512049438),
('lcUptime', '504', 1512049438),
('lcUTC', '1512049436', 1512049438),
('m1FragCnt', '0', 1512049438),
('m1FreeMax', '0', 1512049438),
('m1FreeTot', '3106', 1512049438),
('m1InactCnt', '0', 1512049438),
('m1MemErr', '0', 1512049438),
('m1Uptime', '555', 1512049438),
('m1UTC', '1512049437', 1512049438),
('scan10m', '0', 1512049438),
('scan1d', '0', 1512049438),
('scan1h', '0', 1512049438),
('scan1m', '0', 1512049438),
('smLastLife', '0', 1512049438),
('smLight', '153', 1512049438),
('smLightLP', '171.7356', 1512049438),
('smMemErr', '0', 1512049438),
('smMotion', '0', 1512049438),
('smStatBits', '0', 1512049438),
('smUptime', '3662', 1512049438);

-- --------------------------------------------------------

--
-- Tabellstruktur for tabell `settings`
--

CREATE TABLE `settings` (
  `id` char(50) NOT NULL,
  `value` tinytext,
  `modified` timestamp NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `description` tinytext
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Contains the current settings, no history' ROW_FORMAT=COMPACT;

--
-- Dataark for tabell `settings`
--

INSERT INTO `settings` (`id`, `value`, `modified`, `description`) VALUES
('lcLimit', '160', '2017-11-24 23:03:00', 'Ambient light threshold before activating LED'),
('lcMode', '2', '2017-11-30 13:37:05', 'Controller mode (Off=0/On=1/Auto=2)'),
('lcTEndM', '0', '2017-11-30 13:33:40', 'Minute of day, end of interval to keep light on'),
('lcTStartM', '0', '2017-11-30 13:33:37', 'Minutes of day, start of interval to keep light on'),
('wpixResolution', '-', '2017-11-30 13:20:38', 'Last selected web page plot resolution');

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
-- Dataark for tabell `timeseries`
--

INSERT INTO `timeseries` (`id`, `time`, `scan1m`, `scan10m`, `scan1h`, `scan1d`, `m1UTC`, `m1Uptime`, `m1InactCnt`, `m1FreeTot`, `m1FreeMax`, `m1FragCnt`, `m1MemErr`, `lcUTC`, `lcUptime`, `lcLastLife`, `lcLightOn`, `smUptime`, `smLastLife`, `smMotion`, `smLight`, `smLightLP`) VALUES
(31727, '2017-11-30 14:39:35', 0, 0, 0, 0, 1512049173, 294, 0, 3106, 0, 0, 0, 1512049173, 240, 0, 1, 3398, 0, 0, 112, 143.374),
(31728, '2017-11-30 14:39:45', 1, 0, 0, 0, 1512049183, 304, 0, 3106, 0, 0, 0, 1512049183, 250, 0, 1, 3408, 0, 0, 142, 141.525),
(31739, '2017-11-30 14:41:36', 0, 0, 0, 0, 1512049293, 414, 0, 3106, 0, 0, 0, 1512049293, 360, 0, 0, 3518, 0, 0, 196, 167.212),
(31740, '2017-11-30 14:41:46', 1, 0, 0, 0, 1512049303, 424, 0, 3106, 0, 0, 0, 1512049303, 370, 0, 0, 3528, 0, 0, 168, 168.245),
(31741, '2017-11-30 14:41:56', 0, 0, 0, 0, 1512049313, 434, 0, 3106, 0, 0, 0, 1512049312, 380, 0, 0, 3538, 0, 0, 203, 169.275),
(31742, '2017-11-30 14:42:06', 0, 0, 0, 0, 1512049326, 444, 0, 3106, 0, 0, 0, 1512049322, 390, 0, 0, 3548, 0, 0, 206, 170.057),
(31743, '2017-11-30 14:42:16', 0, 0, 0, 0, 1512049336, 454, 0, 3106, 0, 0, 0, 1512049332, 400, 0, 0, 3558, 0, 0, 175, 170.018),
(31744, '2017-11-30 14:42:26', 0, 0, 0, 0, 1512049346, 464, 0, 3106, 0, 0, 0, 1512049342, 410, 0, 0, 3568, 0, 0, 147, 170.517),
(31745, '2017-11-30 14:42:36', 0, 0, 0, 0, 1512049356, 474, 0, 3106, 0, 0, 0, 1512049352, 421, 0, 0, 3578, 0, 0, 199, 170.911),
(31746, '2017-11-30 14:42:46', 1, 0, 0, 0, 1512049366, 484, 0, 3106, 0, 0, 0, 1512049362, 434, 0, 0, 3588, 0, 0, 141, 171.387),
(31747, '2017-11-30 14:42:56', 0, 0, 0, 0, 1512049376, 494, 0, 3106, 0, 0, 0, 1512049375, 444, 0, 0, 3602, 0, 0, 196, 171.528),
(31748, '2017-11-30 14:43:06', 0, 0, 0, 0, 1512049386, 504, 0, 3106, 0, 0, 0, 1512049385, 453, 0, 0, 3612, 0, 0, 146, 171.432),
(31749, '2017-11-30 14:43:16', 0, 0, 0, 0, 1512049396, 514, 0, 3106, 0, 0, 0, 1512049396, 463, 0, 0, 3621, 0, 0, 161, 171.559),
(31750, '2017-11-30 14:43:27', 0, 0, 0, 0, 1512049406, 524, 0, 3106, 0, 0, 0, 1512049406, 473, 0, 0, 3631, 0, 0, 164, 171.62),
(31751, '2017-11-30 14:43:38', 0, 0, 0, 0, 1512049417, 535, 0, 3106, 0, 0, 0, 1512049417, 485, 0, 0, 3643, 0, 0, 194, 171.642),
(31752, '2017-11-30 14:43:48', 1, 0, 0, 0, 1512049427, 545, 0, 3106, 0, 0, 0, 1512049426, 494, 1, 0, 3652, 0, 0, 192, 171.702),
(31753, '2017-11-30 14:43:58', 0, 0, 0, 0, 1512049437, 555, 0, 3106, 0, 0, 0, 1512049436, 504, 0, 0, 3662, 0, 0, 153, 171.736);

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
  MODIFY `id` bigint(20) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=31754;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
