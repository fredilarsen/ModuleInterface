-- phpMyAdmin SQL Dump
-- version 4.7.4
-- https://www.phpmyadmin.net/
--
-- Host: 127.0.0.1
-- Generation Time: 26. Nov, 2017 21:25 PM
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
('lcLastLife', '0', 1511727902),
('lcLightOn', '1', 1511727902),
('lcMemErr', '0', 1511727902),
('lcStatBits', '0', 1511727902),
('lcUptime', '258617', 1511727902),
('lcUTC', '1511727899', 1511727902),
('m1FragCnt', '2', 1511727902),
('m1FreeMax', '29', 1511727902),
('m1FreeTot', '2956', 1511727902),
('m1InactCnt', '0', 1511727902),
('m1MemErr', '0', 1511727902),
('m1Uptime', '340476', 1511727902),
('m1UTC', '1511727902', 1511727902),
('scan10m', '0', 1511727902),
('scan1d', '0', 1511727902),
('scan1h', '0', 1511727902),
('scan1m', '0', 1511727902),
('smLastLife', '0', 1511727902),
('smLight', '169', 1511727902),
('smMemErr', '0', 1511727902),
('smMotion', '', 1511727902),
('smStatBits', '0', 1511727902),
('smUptime', '620363', 1511727902);

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
('lcMode', '1', '2017-11-26 19:01:49', 'Controller mode (Off=0/On=1/Auto=2)'),
('lcTEndM', '600', '2017-11-23 20:07:19', 'Minute of day, end of interval to keep light on'),
('lcTStartM', '1080', '2017-11-23 20:07:15', 'Minutes of day, start of interval to keep light on'),
('wpixResolution', '-', '2017-11-26 19:56:41', 'Last selected web page plot resolution');

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
  `smLight` smallint(6) NOT NULL COMMENT 'Measured ambient light'
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='All outputs logged versus time';

--
-- Dataark for tabell `timeseries`
--

INSERT INTO `timeseries` (`id`, `time`, `scan1m`, `scan10m`, `scan1h`, `scan1d`, `m1UTC`, `m1Uptime`, `m1InactCnt`, `m1FreeTot`, `m1FreeMax`, `m1FragCnt`, `m1MemErr`, `lcUTC`, `lcUptime`, `lcLastLife`, `lcLightOn`, `smUptime`, `smLastLife`, `smMotion`, `smLight`) VALUES
(1, '2017-11-26 21:21:49', 0, 0, 0, 0, 1511727708, 340285, 0, 2956, 29, 2, 0, 1511727707, 258425, 0, 1, 620167, 0, 0, 171),
(2, '2017-11-26 21:21:59', 0, 0, 0, 0, 1511727718, 340295, 0, 2956, 29, 2, 0, 1511727718, 258435, 0, 1, 620177, 0, 0, 115),
(3, '2017-11-26 21:22:10', 0, 0, 0, 0, 1511727728, 340305, 0, 2956, 29, 2, 0, 1511727728, 258445, 0, 1, 620187, 0, 0, 135),
(4, '2017-11-26 21:22:20', 1, 0, 0, 0, 1511727738, 340315, 0, 2956, 29, 2, 0, 1511727738, 258455, 0, 1, 620197, 0, 0, 153),
(5, '2017-11-26 21:22:30', 0, 0, 0, 0, 1511727749, 340325, 0, 2956, 29, 2, 0, 1511727748, 258465, 0, 1, 620210, 0, 0, 134),
(6, '2017-11-26 21:22:40', 0, 0, 0, 0, 1511727759, 340335, 0, 2956, 29, 2, 0, 1511727758, 258475, 0, 1, 620220, 0, 0, 162),
(7, '2017-11-26 21:22:50', 0, 0, 0, 0, 1511727769, 340345, 0, 2956, 29, 2, 0, 1511727768, 258485, 0, 1, 620230, 0, 0, 109),
(8, '2017-11-26 21:23:00', 0, 0, 0, 0, 1511727779, 340355, 0, 2956, 29, 2, 0, 1511727778, 258496, 0, 1, 620240, 0, 0, 97),
(9, '2017-11-26 21:23:11', 0, 0, 0, 0, 1511727789, 340365, 0, 2956, 29, 2, 0, 1511727788, 258506, 0, 1, 620250, 0, 0, 123),
(10, '2017-11-26 21:23:21', 1, 0, 0, 0, 1511727799, 340375, 0, 2956, 29, 2, 0, 1511727798, 258516, 0, 1, 620260, 0, 0, 131),
(11, '2017-11-26 21:23:31', 0, 0, 0, 0, 1511727809, 340386, 0, 2956, 29, 2, 0, 1511727808, 258526, 0, 1, 620270, 0, 0, 135),
(12, '2017-11-26 21:23:41', 0, 0, 0, 0, 1511727819, 340396, 0, 2956, 29, 2, 0, 1511727818, 258536, 0, 1, 620281, 0, 0, 115),
(13, '2017-11-26 21:23:51', 0, 0, 0, 0, 1511727829, 340406, 0, 2956, 29, 2, 0, 1511727828, 258546, 0, 1, 620291, 0, 0, 98),
(14, '2017-11-26 21:24:01', 0, 0, 0, 0, 1511727839, 340416, 0, 2956, 29, 2, 0, 1511727839, 258556, 0, 1, 620301, 0, 0, 101),
(15, '2017-11-26 21:24:12', 0, 0, 0, 0, 1511727849, 340426, 0, 2956, 29, 2, 0, 1511727849, 258566, 0, 1, 620311, 0, 0, 141),
(16, '2017-11-26 21:24:22', 1, 0, 0, 0, 1511727861, 340436, 0, 2956, 29, 2, 0, 1511727859, 258576, 0, 1, 620321, 0, 0, 102),
(17, '2017-11-26 21:24:32', 0, 0, 0, 0, 1511727872, 340446, 0, 2956, 29, 2, 0, 1511727869, 258586, 0, 1, 620333, 0, 0, 169),
(18, '2017-11-26 21:24:42', 0, 0, 0, 0, 1511727882, 340456, 0, 2956, 29, 2, 0, 1511727879, 258596, 0, 1, 620343, 0, 0, 129),
(19, '2017-11-26 21:24:52', 0, 0, 0, 0, 1511727892, 340466, 0, 2956, 29, 2, 0, 1511727889, 258607, 0, 1, 620353, 0, 0, 148),
(20, '2017-11-26 21:25:02', 0, 0, 0, 0, 1511727902, 340476, 0, 2956, 29, 2, 0, 1511727899, 258617, 0, 1, 620363, 0, 0, 169);

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
  MODIFY `id` bigint(20) NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=21;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
