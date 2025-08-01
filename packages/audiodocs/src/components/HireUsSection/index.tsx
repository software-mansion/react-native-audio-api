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
        {content ? (
          content
        ) : (
          <>
We’re a software company built around improving developer experience and bringing innovative clients' ideas to life. We're pushing boundaries and delivering high-performance solutions that scale.
          </>
        )}
      </p>

      <div className={styles.hireUsButton}>
        <HomepageButton
          href={href}
          title="Hire us"
          target="_blank"
          backgroundStyling={ButtonStyling.SECTION}
          borderStyling={BorderStyling.SECTION}
        />
      </div>
    </div>
  );
};
