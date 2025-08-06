import {
  ButtonStyling,
  BorderStyling,
  HomepageButton,
} from '@swmansion/t-rex-ui/dist/components/HomepageButton';
import styles from './styles.module.css';

export const HireUsSection = ({
  content,
  href,
}: {
  content?: string;
  href: string;
}) => {
  return (
    <div className={styles.hireUsSectionWrapper}>
      <div className={styles.hireUsTitleContainer}>
        <h2>
          We are <span>Software Mansion</span>
        </h2>
      </div>
      <p className={styles.hireUsSectionBody}>
        We’re a software company built around improving developer experience and bringing innovative clients' ideas to life. We're pushing boundaries and delivering high-performance solutions that scale.
        <br />< br />
        Need help integrating React Native Audio API into your project or want to discuss your ideas?
      </p>

      <div className={styles.hireUsButton}>
        <HomepageButton
          href={href}
          title="Let's talk"
          target="_blank"
          backgroundStyling={ButtonStyling.SECTION}
          borderStyling={BorderStyling.SECTION}
        />
      </div>
    </div>
  );
};
