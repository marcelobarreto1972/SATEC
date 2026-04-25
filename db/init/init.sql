--
-- PostgreSQL database dump
--

\restrict bcHXkirJmqH7y9DSvrJsQ2I6E5e8X75WOO2Ry3od8sJV6S9Z3QxjcDqtpyhxoOf

-- Dumped from database version 17.8 (a48d9ca)
-- Dumped by pg_dump version 18.3

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET transaction_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

--
-- Name: app_schema; Type: SCHEMA; Schema: -; Owner: neondb_owner
--

CREATE SCHEMA app_schema;


ALTER SCHEMA app_schema OWNER TO neondb_owner;

SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- Name: sensor_readings; Type: TABLE; Schema: app_schema; Owner: neondb_owner
--

CREATE TABLE app_schema.sensor_readings (
    id integer NOT NULL,
    humidity double precision NOT NULL,
    temperature double precision NOT NULL,
    reading_date timestamp with time zone NOT NULL,
    topic character varying(255),
    inserted_at timestamp with time zone DEFAULT now()
);


ALTER TABLE app_schema.sensor_readings OWNER TO neondb_owner;

--
-- Name: sensor_readings_id_seq; Type: SEQUENCE; Schema: app_schema; Owner: neondb_owner
--

ALTER TABLE app_schema.sensor_readings ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME app_schema.sensor_readings_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Data for Name: sensor_readings; Type: TABLE DATA; Schema: app_schema; Owner: neondb_owner
--

COPY app_schema.sensor_readings (id, humidity, temperature, reading_date, topic, inserted_at) FROM stdin;
1	45.2	22.5	2026-04-20 10:15:00+00	sensor/living_room	2026-04-21 23:45:16.423311+00
2	50.8	23.1	2026-04-20 11:30:00+00	sensor/kitchen	2026-04-21 23:45:16.423311+00
3	38.6	21.9	2026-04-20 12:45:00+00	sensor/bedroom	2026-04-21 23:45:16.423311+00
4	60.3	24	2026-04-20 14:00:00+00	sensor/bathroom	2026-04-21 23:45:16.423311+00
5	55	23.7	2026-04-20 15:20:00+00	sensor/office	2026-04-21 23:45:16.423311+00
\.


--
-- Name: sensor_readings_id_seq; Type: SEQUENCE SET; Schema: app_schema; Owner: neondb_owner
--

SELECT pg_catalog.setval('app_schema.sensor_readings_id_seq', 5, true);


--
-- Name: sensor_readings sensor_readings_pkey; Type: CONSTRAINT; Schema: app_schema; Owner: neondb_owner
--

ALTER TABLE ONLY app_schema.sensor_readings
    ADD CONSTRAINT sensor_readings_pkey PRIMARY KEY (id);


--
-- Name: SCHEMA app_schema; Type: ACL; Schema: -; Owner: neondb_owner
--

GRANT ALL ON SCHEMA app_schema TO app_user;
GRANT USAGE ON SCHEMA app_schema TO dash_user;


--
-- Name: TABLE sensor_readings; Type: ACL; Schema: app_schema; Owner: neondb_owner
--

GRANT SELECT ON TABLE app_schema.sensor_readings TO dash_user;
GRANT ALL ON TABLE app_schema.sensor_readings TO app_user;


--
-- Name: SEQUENCE sensor_readings_id_seq; Type: ACL; Schema: app_schema; Owner: neondb_owner
--

GRANT ALL ON SEQUENCE app_schema.sensor_readings_id_seq TO app_user;


--
-- Name: DEFAULT PRIVILEGES FOR SEQUENCES; Type: DEFAULT ACL; Schema: app_schema; Owner: neondb_owner
--

ALTER DEFAULT PRIVILEGES FOR ROLE neondb_owner IN SCHEMA app_schema GRANT ALL ON SEQUENCES TO app_user;


--
-- Name: DEFAULT PRIVILEGES FOR FUNCTIONS; Type: DEFAULT ACL; Schema: app_schema; Owner: neondb_owner
--

ALTER DEFAULT PRIVILEGES FOR ROLE neondb_owner IN SCHEMA app_schema GRANT ALL ON FUNCTIONS TO app_user;


--
-- Name: DEFAULT PRIVILEGES FOR TABLES; Type: DEFAULT ACL; Schema: app_schema; Owner: neondb_owner
--

ALTER DEFAULT PRIVILEGES FOR ROLE neondb_owner IN SCHEMA app_schema GRANT SELECT ON TABLES TO dash_user;
ALTER DEFAULT PRIVILEGES FOR ROLE neondb_owner IN SCHEMA app_schema GRANT ALL ON TABLES TO app_user;


--
-- PostgreSQL database dump complete
--

\unrestrict bcHXkirJmqH7y9DSvrJsQ2I6E5e8X75WOO2Ry3od8sJV6S9Z3QxjcDqtpyhxoOf

